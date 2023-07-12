#include "userprog/syscall.h"
#include <stdio.h>
#include <stdarg.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "devices/shutdown.h"
#include "threads/synch.h"
#include "filesys/file.h"
#include "filesys/directory.h"
#include "filesys/inode.h"
#include "filesys/filesys.h"

static void syscall_handler (struct intr_frame *);
static bool syscall_error_handler (void *esp, char **arg_adr);
static void halt (void);
static bool pop_args (int num_args, char **arg_array, void *myESP);
static int write (int fd, const void *buf, unsigned size);
static tid_t exec (const char *cmd_line); 
static int wait (tid_t tid);
static int open (const char *file);
static bool create (const char *file, unsigned initial_size);
static bool remove (const char *file);
static int filesize (int fd);
static int read (int fd, void *buffer, unsigned size);
static unsigned tell (int fd);
static void seek (int fd, unsigned position);
static struct file *fd_to_file (int fd);
static void check_buffer (const void *buf, unsigned size);

static bool chdir (const char *dir);
static bool mkdir (const char *dir);
static bool readdir (int fd, char *name);
static bool isdir (int fd);
static int inumber (int fd);

static int MAX_BUFFER = 300;
static int READDIR_MAX_LEN = 14;
static char *delim = "/";

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  /* Initialize the lock. */
  lock_init (&sys_lock);
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{  
  void *myESP = f->esp;
  
  char **arg_adr = palloc_get_page (001 | 002);

  /* Handle error checking with a helper. */
  int temp = syscall_error_handler (myESP, arg_adr);
  
  /* Get the system call number associate with the call. */
  int syscall_number = *((int *) myESP);

  /* Call the right system call. */
  switch (syscall_number)
    {
      /* Halt. */
      case SYS_HALT:
        palloc_free_page (arg_adr);
        halt ();
        break;

      /* Exit. */ 
      case SYS_EXIT:
        if (pop_args (1, arg_adr, myESP))
        {
          f->eax = arg_adr [0];
          palloc_free_page (arg_adr);
          exit ((int) f->eax);
        }
        break;
        
      /* Exec. */
      case SYS_EXEC:
        if (pop_args (1, arg_adr, myESP))
        {
          if (syscall_error_handler (arg_adr [0], arg_adr))
           {
              tid_t tid = exec (arg_adr [0]);
              f->eax = tid != TID_ERROR ? tid : -1;
           }
        }
        break;

      /* Wait. */
      case SYS_WAIT:
        if (pop_args (1, arg_adr, myESP))
          {
            f->eax = wait (arg_adr [0]);
          }
        break;

      /* Create. */
      case SYS_CREATE:
        if (pop_args (2, arg_adr, myESP))
          {
            if (syscall_error_handler (arg_adr [0], arg_adr))
              {
                f->eax = create (arg_adr [0], arg_adr [1]);
              } 
          }
        break;
    
      /* Remove. */
      case SYS_REMOVE:
        if (pop_args (1, arg_adr, myESP))
          {
            if (syscall_error_handler (arg_adr [0], arg_adr))
              f->eax = remove (arg_adr [0]);
          }
        break;

      /* Open. */
      case SYS_OPEN:
        if (pop_args (1, arg_adr, myESP))
          {
            if (syscall_error_handler (arg_adr [0], arg_adr))
              {
                f->eax = open (arg_adr [0]);
              }
          }
        break;
    
      /* FileSize. */
      case SYS_FILESIZE:
        if (pop_args (1, arg_adr, myESP))
          {
            f->eax = filesize (arg_adr [0]);
          }
        break; 
        
      /* Read. */
      case SYS_READ:
        if (pop_args (3, arg_adr, myESP))
          {
            if (syscall_error_handler (arg_adr [1], arg_adr))
              {
                f->eax = read (arg_adr [0], arg_adr [1], arg_adr [2]);
              }
          }
        break;

      /* Write. */ 
      case SYS_WRITE:
        if (pop_args (3, arg_adr, myESP))
          {
            if (syscall_error_handler (arg_adr [1], arg_adr))
              {
                f->eax = write ((int) arg_adr [0], arg_adr [1],
                                (int) arg_adr [2]);
              }
          }
        break;

      /* Seek. */
      case SYS_SEEK:
        if (pop_args (2, arg_adr, myESP))
          seek (arg_adr [0], arg_adr [1]);
        break;
      
      /* Tell. */
      case SYS_TELL:
        if (pop_args (1, arg_adr, myESP))
          f->eax = tell (arg_adr [0]);
        break;

      /* Close. */ 
      case SYS_CLOSE:
        if (pop_args (1, arg_adr, myESP))
          close (arg_adr [0]);
        break; 
        
      case SYS_CHDIR:
        if (pop_args (1, arg_adr, myESP))
          {
            if (syscall_error_handler (arg_adr [0], arg_adr))
              {
                f->eax = chdir (arg_adr [0]);
              }
          }
        break;

      case SYS_MKDIR:
        if (pop_args (1, arg_adr, myESP))
          {
            if (syscall_error_handler (arg_adr [0], arg_adr))
              {
                f->eax = mkdir (arg_adr [0]);
              }
          }
        break;

      case SYS_READDIR:
        if (pop_args (2, arg_adr, myESP))
          {
            if (syscall_error_handler (arg_adr [1], arg_adr))
              {
                f->eax = readdir (arg_adr [0], arg_adr [1]);
              }
          }
        break;

      case SYS_ISDIR:
        if (pop_args (1, arg_adr, myESP))
          {
            f->eax = isdir (arg_adr [0]);
          }
        break;
    
      case SYS_INUMBER:
        if (pop_args (1, arg_adr, myESP))
          {
            f->eax = inumber (arg_adr [0]);
          }
        break;
      
      default:
        exit (-1);
        break;
    }
  palloc_free_page (arg_adr);
}


static bool
syscall_error_handler (void *esp, char **arg_adr)
{
  /* Get the current thread's page directory. */
  struct thread *t = thread_current ();
  uint32_t *pd = t->pagedir;
  /* Pointer not null, in user memory, has a mapped address. */
  if (esp && is_user_vaddr (esp) && pagedir_get_page (pd, esp) != NULL) 
    return true;

  palloc_free_page (arg_adr);
  exit (-1);
}


static bool
pop_args (int num_args, char **arg_array, void *myESP)
{
  int void_pointer_size = sizeof (void*);
  int i = 1;
  while (i <= num_args)
    {
      if (syscall_error_handler (myESP + (i * void_pointer_size), arg_array))
        memcpy (&arg_array [i - 1], myESP + (i * void_pointer_size),
                sizeof (void*));
      i++;
    }
  return true;
} 

/* Halt System Call. */
static void
halt (void)
{
  shutdown_power_off ();
}

/* Write System Call.
   Return the number of bytes we wrote. */
static int
write (int fd, const void *buf, unsigned size)
{
  check_buffer (buf, size);
  int bytes_written = -1;
  char *buffer_char = (char *) buf;
  /* Write to console. */
  if (fd == 1)
    { 
      bytes_written = 0;
      /* Write a certain amount of bytes at a time. */
      while (size > MAX_BUFFER)
        {
          putbuf (buffer_char, MAX_BUFFER);
          buffer_char += MAX_BUFFER;
          size -= MAX_BUFFER;
          bytes_written += MAX_BUFFER;
        }
      if (size > 0)
        {
          /* Write the remaining bytes. */
          putbuf (buffer_char, size);
          bytes_written += size;
        }
    } 
  else if (fd > 1 && fd < 128)
    {
      struct file *file = fd_to_file (fd);
      /* Check if we can write to the file. */
      if (file && !is_deny_write (file) && !inode_is_dir (file_get_inode (file)))
        {
          lock_acquire (&sys_lock);
          bytes_written = file_write (file, buf, size);
          lock_release (&sys_lock);
        }
    }

  /* WHAT HAPPENS IF WRITE TO STDIN??? */
  return bytes_written;
} 

/* Exec System Call.
   Returns the new program pid. */
static tid_t
exec (const char *cmd_line)
{
  return process_execute (cmd_line);
}

/* Wait System Call. */
static int
wait (tid_t tid) 
{
  /* Everything is in process wait. */
  return process_wait (tid);
}

/* Exit System Call. */
void
exit (int status)
{ 
  char *save;
  char *file_name = strtok_r (thread_current ()->name, " ", &save);
  /* Print out the termination message. */
  printf ("%s: exit(%d)\n", file_name, status);
  thread_current ()->exit_status = status;
  
  thread_exit ();
}

/* Open System Call */
static int
open (const char *file)
{
   if (strlen (file) == 0)
    return -1;
  /* Acquire lock before opening file. */
  lock_acquire (&sys_lock);
  /* Open file using filesys_open call. */
  struct file * file_ptr = filesys_open (file);
  lock_release (&sys_lock);
  
  if (file_ptr == NULL)
    return -1;
  
  /* Get the current thread's fd table and find an open spot. */
  struct thread *current_thread = thread_current ();
  struct file **fd_table = current_thread->fd_array;
  int i;
  for(i = 2; i < MAX_ARGS; i++)
    { 
      if (fd_table [i] == NULL)
        {
          /* We found an open spot! */
          fd_table [i] = file_ptr;
          /* Returns fd index. */
          return i;  
        }
    }
  /* No space in file descriptor array. */
  return -1;
}

/* Create System Call. */
static bool create (const char *file, unsigned initial_size) 
{
  lock_acquire (&sys_lock);
  /* printf ("\n Executing create...\n"); */
  bool success = filesys_create (file, initial_size, false); 
  lock_release (&sys_lock);
  return success;
}

/* Close System Call. */
void close (int fd)
{
  if (fd > 1 && fd < MAX_ARGS)
    {
      struct file *file = fd_to_file (fd);
      if (file)
        {
          lock_acquire (&sys_lock);
          file_close (file);
          lock_release (&sys_lock);
          thread_current ()->fd_array [fd] = NULL; 
        }
    }
}

/* Remove System Call. */
static bool 
remove (const char *name)
{
  lock_acquire (&sys_lock);
  bool result = filesys_remove (name); 
  lock_release (&sys_lock);
  return result;
}

/* Filesize System Call. */
static int 
filesize (int fd)
{
  if (fd > 1 && fd < MAX_ARGS)
    {
      struct file *file = fd_to_file (fd);
      if (!file)
        {
          return -1;
        }
      lock_acquire (&sys_lock);
      int size = file_length (file); 
      lock_release (&sys_lock);
      return size;
    }
  else
    {
      return -1;
    }
}

/* Read System Call. */
static int
read (int fd, void *buffer, unsigned size)
{
  check_buffer (buffer, size);
  int bytes = -1;
  if (fd == 0)
    {
      bytes = input_getc ();
    }
  if (fd > 1 && fd < MAX_ARGS)
    {
      struct file *file = fd_to_file (fd);
      if (!file)
        {
          return -1;
        }
      lock_acquire (&sys_lock);
      bytes = file_read (file, buffer, size); 
      lock_release (&sys_lock);
    }
  return bytes;
}

/* Seek System Call. */
static void 
seek (int fd, unsigned position)
{
  if (fd > 1 && fd < MAX_ARGS)
  {
    struct file *file = fd_to_file (fd);
    if (file)
      {
        lock_acquire (&sys_lock);
        file_seek (file, position);
        lock_release (&sys_lock);
      }
  }
}

/* Tell System Call. */
static unsigned 
tell (int fd)
{
  if (fd > 1 && fd < MAX_ARGS)
  {
    struct file *file = fd_to_file (fd);
    if (!file)
      {
        return -1;
      }
    lock_acquire (&sys_lock);
    int pos = file_tell (file);
    lock_release (&sys_lock);
    return pos;
  }
  else 
    {
      return -1;
    }
}

static struct file 
*fd_to_file (int fd)
{
  struct thread *t = thread_current ();
  struct file **fd_table = t->fd_array;
  struct file *file = fd_table [fd];
  return file;
}

static void 
check_buffer (const void *buf, unsigned size)
{
  char *buffer_char = (char *) buf;
  char *buffer_end = buffer_char + size;
  char **temp;
  while (buffer_char <= buffer_end)
  {
    syscall_error_handler (buffer_char, temp);
    buffer_char += PGSIZE;
  }
  syscall_error_handler (buffer_end, temp);
}

/* Changes the current working directory of the process to dir, 
which may be relative or absolute. Returns true if successful, false on failure. */
static bool 
chdir (const char *dir)
{
  debug ("value of dir: %s", dir);
  struct thread *t = thread_current ();
  struct dir *new_cwd = lookup_dir_in_path (dir);
  /*Case 1: dir is null*/
  if (new_cwd == NULL)
  {
    debug(" NEW CWD NULL ");
    return false;
  }
  /*Case 2: dir = .. go to parent*/
  else if (strcmp (dir, "..") == 0)
    {
      dir_close (t->cwd); 
      t->cwd = get_parent_dir (new_cwd);
      return true;
    }
  /*Case 3: normal case, chdir from current */
  else if (strcmp (dir, ".") != 0)
    {
      t->cwd = new_cwd;
      return true;
    }

  return false;    
}



static bool DEBUG_ME = 0;

void debug(const char *fmt, ...) 
{
  if (DEBUG_ME) {
    va_list myvarargs ;
    va_start(myvarargs, fmt);
    printf ("\n") ;
    vprintf (fmt,myvarargs);
    printf ("\n") ;
    va_end(myvarargs);
  }
}

/* 
Creates the directory named dir, which may be relative or absolute. 
Returns true if successful, false on failure. Fails if dir already exists or 
if any directory name in dir, besides the last, does not already exist. That is, 
mkdir("/a/b/c") succeeds only if /a/b already exists and /a/b/c does not.
*/
static bool 
mkdir (const char *dir)
{
  lock_acquire (&sys_lock);
  bool result = filesys_create (dir, 0, true);
  lock_release (&sys_lock);

  void * ptr = (void *)lookup_dir_in_path (dir) ;


  debug("\n Result : %d [0x%0x]\n", result, ptr) ;


  return result;
}

/*
Reads a directory entry from file descriptor fd, 
which must represent a directory. If successful, stores 
the null-terminated file name in name, which 
must have room for READDIR_MAX_LEN + 1 bytes, 
and returns true. If no entries are left in the directory, 
returns false.
 */
static bool 
readdir (int fd, char *name)
{

   /* How to check if name has room for
     READDIR_MAX_LEN + 1 bytes
   */
  if (!isdir (fd) || strlen (name) < (READDIR_MAX_LEN + 1))
      return false;

  struct file *file = fd_to_file (fd);
  struct inode *inode = file_get_inode (file);
  struct dir *dir = dir_open (inode);

  return dir_readdir (dir, name);
}

/* Returns true if fd represents a directory, 
   false if it represents an ordinary file. */
static bool 
isdir (int fd)
{
  struct file *file = fd_to_file (fd);
  struct inode *inode = file_get_inode (file);
  return inode_is_dir (inode);
}

/* Returns the inode number of the inode associated with fd, 
   which may represent an ordinary file or a directory. */
static int 
inumber (int fd)
{
  struct file *file = fd_to_file (fd);
  struct inode *inode = file_get_inode (file);
  return inode_get_inumber (inode);
}
