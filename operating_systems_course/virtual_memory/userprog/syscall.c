
#include "userprog/syscall.h"

static void syscall_handler (struct intr_frame *);
static bool syscall_error_handler (void *esp, char **arg_adr);
static void halt (void);
static bool pop_args (int num_args, char **arg_array, void *myESP);
static int write (int fd, const void *buf, unsigned size, struct intr_frame *f);
static tid_t exec (const char *cmd_line); 
static int wait (tid_t tid);
static int open (const char *file);
static bool create (const char *file, unsigned initial_size);
static bool remove (const char *file);
static int filesize (int fd);
static int read (int fd, void *buffer, unsigned size, struct intr_frame *f);
static unsigned tell (int fd);
static void seek (int fd, unsigned position);
static struct file *fd_to_file (int fd);
static void check_buffer (const void *buf, unsigned size, struct intr_frame *f);
static bool grow_stack_addr (void *esp, struct intr_frame *f);
static void release_pages (const void *buf, unsigned size);

int MAX_BUFFER = 300;

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
  /* Everyone drove here. */
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
      case 0:
        palloc_free_page (arg_adr);
        halt ();
        break;

      /* Exit. */ 
      case 1:
        if (pop_args (1, arg_adr, myESP))
        {
          f->eax = arg_adr [0];
          palloc_free_page (arg_adr);
          exit ((int) f->eax);
        }
        break;
        
      /* Exec. */
      case 2:
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
      case 3:
        if (pop_args (1, arg_adr, myESP))
          {
            f->eax = wait (arg_adr [0]);
          }
        break;

      /* Create. */
      case 4:
        if (pop_args (2, arg_adr, myESP))
          {
            if (syscall_error_handler (arg_adr [0], arg_adr))
              {
                f->eax = create (arg_adr [0], arg_adr [1]);
              } 
          }
        break;
    
      /* Remove. */
      case 5:
        if (pop_args (1, arg_adr, myESP))
          {
            if (syscall_error_handler (arg_adr [0], arg_adr))
              f->eax = remove (arg_adr [0]);
          }
        break;

      /* Open. */
      case 6:
        if (pop_args (1, arg_adr, myESP))
          {
            if (syscall_error_handler (arg_adr [0], arg_adr))
              {
                f->eax = open (arg_adr [0]);
              }
          }
        break;
    
      /* FileSize. */
      case 7:
        if (pop_args (1, arg_adr, myESP))
          {
            f->eax = filesize (arg_adr [0]);
          }
        break; 
        
      /* Read. */
      case 8:
        if (pop_args (3, arg_adr, myESP))
          {        
            f->eax = read (arg_adr [0], arg_adr [1], arg_adr [2], f);   
          }
        break;

      /* Write. */ 
      case 9:
        if (pop_args (3, arg_adr, myESP))
          {
            if (syscall_error_handler (arg_adr [1], arg_adr))
              {
                f->eax = write ((int) arg_adr [0], arg_adr [1],
                                (int) arg_adr [2], f);
              }
          }
        break;

      /* Seek. */
      case 10:
        if (pop_args (2, arg_adr, myESP))
          seek (arg_adr [0], arg_adr [1]);
        break;
      
      /* Tell. */
      case 11:
        if (pop_args (1, arg_adr, myESP))
          tell (arg_adr [0]);
        break;

      /* Close. */ 
      case 12:
        if (pop_args (1, arg_adr, myESP))
          close (arg_adr [0]);
        break; 
      
      default:
        exit (-1);
        break;
    }
  palloc_free_page (arg_adr);
}

/* A function for error handling the user pointer. 
   Checks for null, kernel, or invalid pointer.
*/
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

/* Getting the arguments from the stack and saving it in the array.
*/
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
write (int fd, const void *buf, unsigned size, struct intr_frame *f)
{
  check_buffer (buf, size, f);
  int bytes_written = -1;
  char *copy_of_buffer = buf;
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
      if (!file)
        {
          release_pages (copy_of_buffer, size);
          return -1;
        }
      /* Check if we can write to the file. */
      if (is_deny_write(file))
        return bytes_written;
      lock_acquire (&sys_lock);
      bytes_written = file_write (file, buf, size); 
      lock_release (&sys_lock);
    }
  release_pages (copy_of_buffer, size);
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
  /* Acquire lock before opening file. */
  lock_acquire (&sys_lock);
  /* Open file using filesys_open call. */
  struct file * file_ptr = filesys_open (file);
  lock_release (&sys_lock);

  if (file_ptr == NULL)
    return -1;
  
  /* Get the current thread's fd table and find an open spot. */
  struct thread *current_thread = thread_current ();
  struct file **fd_table = current_thread->fdt;
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
  bool success = filesys_create (file, initial_size); 
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
          thread_current ()->fdt [fd] = NULL; 
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
read (int fd, void *buffer, unsigned size, struct intr_frame *f)
{
  if (!buffer)
    return -1;
  check_buffer (buffer, size, f);
  char *copy_of_buffer = buffer;
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
          release_pages (copy_of_buffer, size);
          return -1;
        }
      lock_acquire (&sys_lock);
      bytes = file_read (file, buffer, size);
      lock_release (&sys_lock);
    }
  release_pages (copy_of_buffer, size);
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

/* Helper to convert fd to struct file. */
static struct file 
*fd_to_file (int fd)
{
  struct thread *t = thread_current ();
  struct file **fd_table = t->fdt;
  struct file *file = fd_table [fd];
  return file;
}

static void 
check_buffer (const void *buf, unsigned size, struct intr_frame *f)
{
  char *buffer_char = (char *) buf;
  char *buffer_end = buffer_char + size;
  struct page *p;
  while (buffer_char <= buffer_end)
  {
    p = lookup (&thread_current ()->supp_page_table, pg_round_down (buffer_char));
    if (p && !p->is_writable)
    {
      exit (-1);
    }
    if (p)
    {
      lock_acquire (&p->pin);
    }
    if (!grow_stack_addr (buffer_char, f))
    {
      exit (-1);
    } 

    buffer_char += PGSIZE;
  }
  if (!grow_stack_addr (buffer_end, f))
  {
    exit (-1);
  }
}

/* Function to grow stack in check buffer. */
static bool
grow_stack_addr (void *esp, struct intr_frame *f)
{
  if (esp && is_user_vaddr (esp))
    {
      uint32_t *pd = thread_current ()->pagedir;

      if (pagedir_get_page (pd, esp) != NULL)
        return true;

      struct page *p = stack_growth (esp, f);
      if (p)
        {
          void *f_ptr = insert_ft (002 | 004);

          /* (PAGE TABLE) */
          if (install_page (pg_round_down (esp), f_ptr, p->is_writable) &&
              load_page (f_ptr, p))
            {
              lock_acquire (&p->pin);
              return true;
            }
          remove_ft (p);
        }
    }
  return false;
}

/* Release all the locks that the pages are holding. */
static void 
release_pages (const void *buf, unsigned size)
{
  char *buffer_char = (char *) buf;
  char *buffer_end = buffer_char + size;
  struct page *p;
  while (buffer_char <= buffer_end)
  {
    p = lookup (&thread_current ()->supp_page_table, pg_round_down (buffer_char));
    
    if (p && lock_held_by_current_thread (&p->pin))
      lock_release (&p->pin);

    buffer_char += PGSIZE;
  }

  p = lookup (&thread_current ()->supp_page_table, pg_round_down (buffer_end));
  if (p && lock_held_by_current_thread (&p->pin))
      lock_release (&p->pin);

}