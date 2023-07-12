#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44
#define DIRECT_NUMBER 10
#define INDIRECT_NUMBER 1
#define INDIRECT_PTRS 128
#define DOUBLE_INDIRECT_NUM 1
#define NOT_DOUBLE (DIRECT_NUMBER + (INDIRECT_PTRS * INDIRECT_NUMBER))
#define UNIT_ALLOC 1
#define ERROR_FIND -1
#define MAX_BLOCKS (INDIRECT_PTRS * INDIRECT_PTRS)
static int zeros[INDIRECT_PTRS];
static int init_done = 0;

struct indirect
{
    block_sector_t data_ptrs [INDIRECT_PTRS]; 
};

struct inode_disk
{
    block_sector_t direct [DIRECT_NUMBER];     /* ten blocks of data */
    block_sector_t double_re_indirect;
    block_sector_t single_re_indirect;                  

    off_t length;                       /* File size in bytes. */
    unsigned magic;                     /* Magic number. */

    /* is inode dir or file */
    bool is_dir;
    /* if inode is a dir, this is num of entries */                        
    int entries_number;                   
    block_sector_t parent_dir;
    
    uint32_t unused[(INDIRECT_PTRS - DIRECT_NUMBER - INDIRECT_NUMBER - 
                     DOUBLE_INDIRECT_NUM - 5)];
};

static int create_indir (int start, int bound, struct indirect *ind, struct inode_disk *disk_inode, block_sector_t ind_sector, block_sector_t sector);

static int make_blocks_num (int new_len, int orig_len);

static bool grow_file (struct inode *inode, int make_blocks_num, int size);

/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, BLOCK_SECTOR_SIZE);
}

/* In-memory inode. */
struct inode 
  {
    struct list_elem elem;              /* Element in inode list. */
    block_sector_t sector;              /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
    struct inode_disk data;             /* Inode content. */
    bool is_dir;                        /* Tells us if directory or file. */
    int entries_number;                    /* Number of entries open if directory. */
    block_sector_t parent_dir;          /* Block sector of parent inode. */
  };

/* Returns the block device sector that contains byte offset POS
   within INODE.
   Returns -1 if INODE does not contain data for a byte at offset
   POS. */
static block_sector_t
byte_to_sector (const struct inode *inode, off_t pos) 
{
  ASSERT (inode != NULL);
   size_t sectors = pos / BLOCK_SECTOR_SIZE;
   if (pos > inode->data.length){
    return -1;
   }
   
   block_sector_t block;
   if (sectors < DIRECT_NUMBER)
    {
        block = inode->data.direct [sectors];
    }
    else if (sectors < NOT_DOUBLE)
      {
        block_sector_t will_read_block = inode->data.single_re_indirect;
        struct indirect *single_block = calloc(1, sizeof (struct indirect));
        if (single_block == NULL)
          return -1;

        block_read (fs_device, will_read_block, single_block->data_ptrs);
        ASSERT (inode->data.magic == INODE_MAGIC);
        int sector_return = sectors - DIRECT_NUMBER;
        block = single_block->data_ptrs[sector_return];
        free (single_block);
      }
    else
      {
         size_t double_indirect_index = (sectors - NOT_DOUBLE) / (INDIRECT_PTRS);
         block_sector_t will_read_block = inode->data.double_re_indirect;
         struct indirect *double_block = calloc (1, sizeof (struct indirect));

         block_read (fs_device, will_read_block, double_block);
         ASSERT (inode->data.magic == INODE_MAGIC);
         struct indirect *single_block = calloc (1, sizeof (struct indirect));
         block_read (fs_device, double_block->data_ptrs[double_indirect_index], single_block);
         ASSERT (inode->data.magic == INODE_MAGIC);
         will_read_block = (sectors - NOT_DOUBLE) % (INDIRECT_PTRS);
         block = single_block->data_ptrs [will_read_block];
         free (double_block);
         free (single_block);
      }
      return block;
}

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/* Initializes the inode module. */
void
inode_init (void) 
{
  list_init (&open_inodes);
}


static int
create_indir (int start, int bound, struct indirect *ind, struct inode_disk *disk_inode,
               block_sector_t ind_sector, block_sector_t sector)
{
  int single_indirect_idx;
  int number_blocks_wrote = 0;
  
  for (single_indirect_idx = start; single_indirect_idx < bound && single_indirect_idx < INDIRECT_PTRS; single_indirect_idx++)
    {
      if (free_map_allocate (1, &ind->data_ptrs[single_indirect_idx]) == false)
        {
          block_write (fs_device, sector, disk_inode);
          return -1;
        }
      block_write (fs_device, ind->data_ptrs[single_indirect_idx], zeros);
      number_blocks_wrote++;
    }
  block_write (fs_device, ind_sector, ind->data_ptrs);
  
  return number_blocks_wrote;
}

/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   device.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool
inode_create (block_sector_t sector, off_t length, bool is_dir)
{
  struct inode_disk *disk_inode = NULL;

  ASSERT (length >= 0);

  /* If this assertion fails, the inode structure is not exactly
     one sector in size, and you should fix that. */
  ASSERT (sizeof *disk_inode == BLOCK_SECTOR_SIZE);
  int loop;
  if (!init_done)
    {
      for(loop = 0; loop < INDIRECT_PTRS; loop++)
        zeros[loop] = 0;
      init_done = 1;
    }

  disk_inode = calloc (1, sizeof *disk_inode);
  
  if (disk_inode == NULL)
    return false;
  
  size_t sectors = bytes_to_sectors (length);
  disk_inode->magic = INODE_MAGIC;
  disk_inode->length = length;
  disk_inode->is_dir = is_dir;
  disk_inode->parent_dir = ROOT_DIR_SECTOR;
  disk_inode->entries_number = 0;
  if (is_dir == false)
    disk_inode->entries_number = -1;
  int direct_idx;
  int bound = sectors < DIRECT_NUMBER ? sectors : DIRECT_NUMBER;

  for (direct_idx = 0; direct_idx < bound; direct_idx++)
    {
        if (free_map_allocate (1, &disk_inode->direct[direct_idx]) == false)
        {
          block_write (fs_device, sector, disk_inode);
          return false;
        }
        block_write (fs_device, disk_inode->direct[direct_idx], &zeros);
    }
  if (sectors < DIRECT_NUMBER)
    {
      block_write (fs_device, sector, disk_inode);
      return true;
    }
  
  struct indirect *single_ind = NULL;
  single_ind = calloc (1, sizeof (struct indirect));

  if (single_ind == NULL)
    return false;
  
  if (!free_map_allocate (1, &disk_inode->single_re_indirect))
    {
      block_write (fs_device, sector, disk_inode);
      free (single_ind);
      return false;
    }

  bound = sectors < NOT_DOUBLE ? (sectors - DIRECT_NUMBER) : INDIRECT_PTRS;
  int check_allocation = create_indir (0, bound, single_ind, disk_inode, 
                                   disk_inode->single_re_indirect, sector);
  if (check_allocation == -1)
    {
      free (single_ind);
      return false;
    }

  if (sectors < NOT_DOUBLE)
    {
      block_write (fs_device, sector, disk_inode);
      free (single_ind);
      return true;
    }
  free (single_ind);

  struct indirect *double_ind = NULL;
  double_ind = calloc (1, sizeof (struct indirect));

  if (!free_map_allocate (1, &disk_inode->double_re_indirect))
  {
    block_write (fs_device, sector, disk_inode);
    free (double_ind);
    return false;
  }

  int dble_idx;
  int ptr_idx = 0;
  size_t sectors_left = sectors - NOT_DOUBLE;
  for (dble_idx = NOT_DOUBLE; dble_idx < sectors; dble_idx += INDIRECT_PTRS)
    {
      struct indirect *ind = NULL;
      ind = calloc (1, sizeof (struct indirect));

      if (ind == NULL)
        {
          free (double_ind);
          free (ind);
          return false;
        }
      
      if (!free_map_allocate (1, &double_ind->data_ptrs[ptr_idx]))
        {
          block_write (fs_device, sector, disk_inode);
          free (double_ind);
          free (ind);
          return false;
        }
      bound = sectors_left < INDIRECT_PTRS ? sectors_left : INDIRECT_PTRS;
      
      check_allocation = create_indir (0, bound, ind, disk_inode, 
                                            double_ind->data_ptrs[ptr_idx], sector);

      if (check_allocation == -1)
        {
          free (double_ind);
          free (ind);
          return false;
        }
        
      ptr_idx ++;
      sectors_left -= INDIRECT_PTRS;
    }

    block_write (fs_device, disk_inode->double_re_indirect, &double_ind->data_ptrs);

    block_write (fs_device, sector, disk_inode);
    free (double_ind);
    free (disk_inode);

    return true;
}

/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (block_sector_t sector)
{
  struct list_elem *list_e;
  struct inode *inode;

  /* Check whether this inode is already open. */
  for (list_e = list_begin (&open_inodes); list_e != list_end (&open_inodes);
       list_e = list_next (list_e)) 
    {
      inode = list_entry (list_e, struct inode, elem);
      if (inode->sector == sector) 
        {
          inode_reopen (inode);
          return inode; 
        }
    }

  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL)
    return NULL;

  /* Initialize. */
  list_push_front (&open_inodes, &inode->elem);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
  block_read (fs_device, inode->sector, &inode->data);
  inode->is_dir = inode->data.is_dir;
  inode->parent_dir = inode->data.parent_dir;
  inode->entries_number = inode->data.entries_number;
  return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
  if (inode != NULL)
    inode->open_cnt++;
  return inode;
}

/* Returns INODE's inode number. */
block_sector_t
inode_get_inumber (const struct inode *inode)
{
  return inode->sector;
}

/* Closes INODE and writes it to disk. (Does it?  Check code.)
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode) 
{
  /* Ignore null pointer. */
  if (inode == NULL)
    return;

  /* Release resources if this was the last opener. */
  if (--inode->open_cnt == 0)
    {
      /* Remove from inode list and release lock. */
      list_remove (&inode->elem);
 
      /* Deallocate blocks if removed. */
      if (inode->removed) 
        {
          free_map_release (inode->sector, 1);
          size_t sectors = bytes_to_sectors (inode->data.length);
          int direct_idx;
          
          int bound = sectors < DIRECT_NUMBER ? sectors : DIRECT_NUMBER;
          for (direct_idx = 0; direct_idx < bound; direct_idx++)
            {
              free_map_release (inode->data.direct [direct_idx], 1);
            }
          
          block_sector_t will_read_block = inode->data.single_re_indirect;
          struct indirect *single_block = calloc(1, sizeof (struct indirect));
          
          if (single_block == NULL)
            return;
          
          block_read (fs_device, will_read_block, single_block);
          bound = sectors < NOT_DOUBLE ? (sectors - DIRECT_NUMBER) : INDIRECT_PTRS;
          
          int single_indirect_idx;
          free_map_release (inode->data.single_re_indirect, 1);
          for (single_indirect_idx = 0; single_indirect_idx < bound; single_indirect_idx++)
            {
              free_map_release (single_block->data_ptrs[single_indirect_idx], 1);  
            }
          free (single_block);
          
          will_read_block = inode->data.double_re_indirect;
          struct indirect *double_block = calloc (1, sizeof (struct indirect));
          if (double_block == NULL)
            return;
          block_read (fs_device, will_read_block, double_block);

          size_t sectors_left = sectors - NOT_DOUBLE;
          int data_index = 0;
          int dble_idx;
          for (dble_idx = 0; dble_idx < sectors_left; dble_idx += INDIRECT_PTRS)
            {
              struct indirect *block = calloc (1, sizeof (struct indirect));
              if (block == NULL)
                return;
              block_read (fs_device, double_block->data_ptrs[data_index], block);
              int sgl_index;
              for (sgl_index = 0; sgl_index < INDIRECT_PTRS; sgl_index ++)
                {
                    free_map_release (block->data_ptrs [sgl_index], 1);
                }
              free_map_release (double_block->data_ptrs [data_index], 1);
              free (block);
              data_index++;
            }
          free (double_block);
        }

      free (inode); 
    }
}

/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *inode) 
{
  ASSERT (inode != NULL);
  inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset) 
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;
  uint8_t *bounce = NULL;

  while (size > 0) 
    {
      /* Disk sector to read, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Read full sector directly into caller's buffer. */
          block_read (fs_device, sector_idx, buffer + bytes_read);
        }
      else 
        {
          /* Read sector into bounce buffer, then partially copy into caller's buffer. */
          if (bounce == NULL) 
            {
              bounce = malloc (BLOCK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }
          block_read (fs_device, sector_idx, bounce);
          memcpy (buffer + bytes_read, bounce + sector_ofs, chunk_size);
        }
      
      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_read += chunk_size;
    }
  free (bounce);

  return bytes_read;
}

static bool
grow_file (struct inode *inode, int make_blocks_num, int size)
{
  struct inode_disk *disk_inode = &inode->data;
  int num_sectors = DIV_ROUND_UP (disk_inode->length, BLOCK_SECTOR_SIZE);
  inode->data.length = size;

  int bound;
  int i;
  int blocks_made;
  
  if (num_sectors < DIRECT_NUMBER && make_blocks_num > 0) 
    {
      for (i = num_sectors; i < DIRECT_NUMBER && make_blocks_num > 0; i++)
        {
          
          if (!free_map_allocate (1, &disk_inode->direct[i]))
            {
              block_write (fs_device, inode->sector, disk_inode);
              return false;
            }
          block_write (fs_device, disk_inode->direct[i], &zeros);
          make_blocks_num --;
        }
    }
  
  if (num_sectors < NOT_DOUBLE && make_blocks_num > 0)
    { 
      i = num_sectors < DIRECT_NUMBER ? 0 : (num_sectors - DIRECT_NUMBER);
      struct indirect *ind = calloc (1, sizeof (struct indirect));
      if (num_sectors <= DIRECT_NUMBER)
        {
          if (!free_map_allocate (1, &disk_inode->single_re_indirect))
            {
              block_write (fs_device, inode->sector, disk_inode);
              return false;
            }
        } 
      else
        {
          block_read (fs_device, disk_inode->single_re_indirect, ind->data_ptrs);
        }
      blocks_made = create_indir (i, i + make_blocks_num, ind, disk_inode, disk_inode->single_re_indirect, inode->sector);
      free (ind);
      if (blocks_made == -1)
      {
        return false;
      }
      make_blocks_num -= blocks_made;
    }

  if (num_sectors < MAX_BLOCKS && make_blocks_num > 0)
    {
      struct indirect *double_block = NULL;
      double_block = calloc (1, sizeof (struct indirect));
      if (!double_block)
        return false;


      struct indirect *sgl_ind = NULL;
      int indirect_index;

      if (num_sectors <= NOT_DOUBLE)
        {
          if (!free_map_allocate (1, &disk_inode->double_re_indirect))
            {
              block_write (fs_device, inode->sector, disk_inode);
              free (double_block);
              return false;
            }

          int indirect_bound = DIV_ROUND_UP(make_blocks_num, INDIRECT_PTRS);
          for (indirect_index = 0; indirect_index < indirect_bound; indirect_index++)
            {
              sgl_ind = calloc (1, sizeof (struct indirect));
              if (sgl_ind == NULL)
                {
                  free (double_block);
                  return false;
                }
              if (!free_map_allocate (1, &double_block->data_ptrs[indirect_index]))
                {
                  block_write (fs_device, inode->sector, disk_inode);
                  free (double_block);
                  free (sgl_ind);
                  return false;
                }

              blocks_made = create_indir (0, make_blocks_num, sgl_ind, disk_inode, 
                                           double_block->data_ptrs[indirect_index], inode->sector);
              if (blocks_made == -1)
                {
                  free (double_block);
                  free (sgl_ind);
                  return false;
                }
              make_blocks_num -= blocks_made;
              free (sgl_ind);
              sgl_ind = NULL;
            }
          block_write (fs_device, disk_inode->double_re_indirect, &double_block->data_ptrs);
          free (double_block);
        }
      else
        {
          int indirect_num = (num_sectors - NOT_DOUBLE) / INDIRECT_PTRS; 
          int start_data_block = ((num_sectors - NOT_DOUBLE) % INDIRECT_PTRS);
          
          block_read (fs_device, disk_inode->double_re_indirect, double_block->data_ptrs);

          int i;
          for (indirect_index = indirect_num; (indirect_index < INDIRECT_PTRS && make_blocks_num > 0); indirect_index++)
            {
              sgl_ind = calloc (1, sizeof (struct indirect));
              if (sgl_ind == NULL)
                {
                  free (double_block);
                  return false;
                }
              
              int start = 0;
              if (indirect_index == indirect_num) 
                {
                  block_read (fs_device, double_block->data_ptrs[indirect_index], sgl_ind->data_ptrs);
                  start = start_data_block;
                }
              else 
                {
                   if (!free_map_allocate (1, &double_block->data_ptrs[indirect_index]))
                    {
                      block_write (fs_device, inode->sector, disk_inode);
                      free (double_block);
                      free (sgl_ind);
                      return false;
                    }
                }
              
              blocks_made = create_indir (start_data_block, start_data_block + make_blocks_num, sgl_ind, disk_inode, 
                                           double_block->data_ptrs[indirect_index], inode->sector);
              if (blocks_made == -1)
                {
                  free (double_block);
                  free (sgl_ind);
                  return false;
                }
              make_blocks_num -= blocks_made;
              free (sgl_ind); 
              sgl_ind = NULL;
            }
          block_write (fs_device, disk_inode->double_re_indirect, &double_block->data_ptrs);
          free (double_block);
        }
    }
    
  block_write (fs_device, inode->sector, disk_inode);
  return true;

}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset) 
{
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;
  uint8_t *bounce = NULL;

  if (inode->deny_write_cnt)
    return 0;

  int blocks = make_blocks_num (offset + size, inode->data.length);
  
  /* Check if offset is past size. */
  if (inode_length (inode) < offset + size)
    grow_file (inode, blocks, offset + size);
  while (size > 0) 
    {
      /* Sector to write, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually write into this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
      {
        break;
      }

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Write full sector directly to disk. */
          block_write (fs_device, sector_idx, buffer + bytes_written);
        }
      else 
        {
          /* We need a bounce buffer. */
          if (bounce == NULL) 
            {
              bounce = malloc (BLOCK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }

          /* If the sector contains data before or after the chunk
             we're writing, then we need to read in the sector
             first.  Otherwise we start with a sector of all zeros. */
          if (sector_ofs > 0 || chunk_size < sector_left) 
            block_read (fs_device, sector_idx, bounce);
          else
            memset (bounce, 0, BLOCK_SECTOR_SIZE);
          memcpy (bounce + sector_ofs, buffer + bytes_written, chunk_size);
          block_write (fs_device, sector_idx, bounce);
        }
      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_written += chunk_size;
    }
  free (bounce);

  return bytes_written;
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode) 
{
  inode->deny_write_cnt++;
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode) 
{
  ASSERT (inode->deny_write_cnt > 0);
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  inode->deny_write_cnt--;
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
  return inode->data.length;
}

bool
inode_is_dir (const struct inode *inode)
{
  return inode->is_dir;
}

bool
inode_in_use (const struct inode *inode)
{
  return inode->open_cnt > 1;
}

bool
inode_set_parent (block_sector_t parent, block_sector_t child)
{
  struct inode *child_inode = inode_open (child);
  if (!child_inode)
    return false;
  
  child_inode->parent_dir = parent;
  inode_close (child_inode);

  return true;
}

block_sector_t
inode_get_parent (const struct inode *inode)
{
  return inode->parent_dir;
}

static int
make_blocks_num (int new_len, int orig_len)
{
  new_len = DIV_ROUND_UP (new_len, BLOCK_SECTOR_SIZE);
  orig_len = DIV_ROUND_UP (orig_len, BLOCK_SECTOR_SIZE);
  return new_len - orig_len;
}

int 
get_num_entries (struct inode *inode)
{
  return inode->entries_number;
}

void
inc_num_entries (struct inode *inode)
{
  inode->entries_number ++;
}

void
dec_num_entries (struct inode *inode)
{
  inode->entries_number --;
}
