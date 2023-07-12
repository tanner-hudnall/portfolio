#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "threads/thread.h"
#include "threads/palloc.h"

/* Partition that contains the file system. */
struct block *fs_device;

static void do_format (void);
static char *slash_delimeter = "/";
//static char delim_char = '/';

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format) 
{
  fs_device = block_get_role (BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC ("No file system device found, can't initialize file system.");

  inode_init ();
  free_map_init ();

  if (format) 
    do_format ();

  free_map_open ();
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void) 
{
  free_map_close ();
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create (const char *name, off_t initial_size, bool is_dir) 
{
  block_sector_t inode_sector = 0;
  debug ("Current working directory is not NULL: %d", (thread_current ()->cwd != NULL));
  struct dir *directory = (thread_current ()->cwd != NULL) ? thread_current ()->cwd : dir_open_root ();
  struct dir *last_directory = lookup_dir_in_path (name);
  debug ("The name is %s", name);
  if (last_directory)
  {
    debug ("Already exists");
    return false;
  }
  char tok_path [strlen (name) + 1];
  strlcpy (tok_path, name, strlen(name) + 1);
  char *save_ptr = NULL;
  char *start_of_parent_dir = NULL;
  char *moded_name = strtok_r (tok_path, slash_delimeter, &save_ptr); //modified name

  char *old_name; 
  while (moded_name != NULL)
    {
      old_name = moded_name;
      moded_name = strtok_r (NULL, slash_delimeter, &save_ptr);
    }
  debug("old_name = %s", old_name);
  if (abs_or_rel (name))
    {
      int path_len = (strlen (name) + 1) - (strlen (old_name) + 1);
      
      char *dir_path = NULL;
      dir_path = palloc_get_page (001 | 002) ;
      if (!dir_path)
        return false;
        
      if(path_len != 0)
        strlcpy (dir_path, name, path_len);
      else
        strlcpy (dir_path, slash_delimeter, 2);
      debug("dir_path = %s", dir_path);
      start_of_parent_dir = dir_path;
      struct dir *parent_dir = NULL;
      parent_dir = lookup_dir_in_path (dir_path);
      if (!parent_dir)
        return false;
      
      directory = dir_open (dir_get_inode (parent_dir));
    } 
    else
    {
      directory = dir_open (dir_get_inode (directory));
    }

  bool success = (directory != NULL
              && free_map_allocate (1, &inode_sector)
              && inode_create (inode_sector, initial_size, is_dir)
              && dir_add (directory, old_name, inode_sector, is_dir));
  

  if (!success && inode_sector != 0) 
    free_map_release (inode_sector, 1);
  dir_close (directory);
  palloc_free_page (start_of_parent_dir);
  return success;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *name)
{
  struct dir *dir = lookup_dir_in_path (name);
  struct inode *inode = NULL;
  /* Open directory */
  if (dir != NULL)
    {
      inode = dir_get_inode (dir);
      if (inode == NULL)
        return NULL;
      return file_open (inode);
    }
  struct dir *parent_dir = (thread_current ()->cwd != NULL) ? thread_current ()->cwd : dir_open_root ();
  char tok_path [strlen (name) + 1];
  strlcpy (tok_path, name, strlen(name) + 1);
  char *save_ptr = NULL;
  char *moded_name = strtok_r (tok_path, slash_delimeter, &save_ptr);
  char *old_name;
  while (moded_name != NULL)
    {
      old_name = moded_name;
      moded_name = strtok_r (NULL, slash_delimeter, &save_ptr);
    }


  int path_len = (strlen (name) + 1) - (strlen (old_name) + 1);
  
  char *dir_path = NULL;
  dir_path = palloc_get_page (001 | 002);
  if (!dir_path)
    return false;
    
  if(path_len != 0)
    strlcpy (dir_path, name, path_len);
  else
    strlcpy (dir_path, slash_delimeter, 2);

  char *start_of_parent_dir = dir_path;
  parent_dir = lookup_dir_in_path (dir_path);
  
  dir_open (dir_get_inode (parent_dir));
  dir_lookup (parent_dir, old_name, &inode);
  dir_close (parent_dir);
  palloc_free_page (start_of_parent_dir);
  return file_open (inode);
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name) 
{
  char tok_path [strlen (name) + 1];
  strlcpy (tok_path, name, strlen(name) + 1);
  char *save_ptr = NULL;
  char *moded_name = strtok_r (tok_path, slash_delimeter, &save_ptr);
  char *old_name;
  while (moded_name != NULL)
    {
      old_name = moded_name;
      moded_name = strtok_r (NULL, slash_delimeter, &save_ptr);
    }

  int path_len = (strlen (name) + 1) - (strlen (old_name) + 1);  
  char *dir_path = NULL;
  dir_path = palloc_get_page (001 | 002);
  char *dir_path_head = dir_path;
  if (!dir_path)
    return false;
  debug ("Path size: %d", path_len);
  if(path_len != 0)
    {
      debug ("Name: %s", name);
      strlcpy (dir_path, name, path_len + 1);
    }
  else
    strlcpy (dir_path, slash_delimeter, 2);
  
  debug ("Directory path: %s", name);

  struct dir *dir = lookup_dir_in_path (name);

  debug ("Dir/File Name: %s", old_name);

  bool success = dir != NULL && dir_remove (dir, old_name);
  dir_close (dir); 
  palloc_free_page (dir_path_head);
  return success;
}

/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_create (ROOT_DIR_SECTOR, 16))
    PANIC ("root directory creation failed");
  free_map_close ();
  printf ("done.\n");
}

