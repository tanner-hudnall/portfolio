
#include "userprog/process.h"

static thread_func start_process NO_RETURN;
static bool load (const char *cmdline, void (**eip) (void), void **esp);
static bool space_on_stack (char *esp, int arg_len);
static struct thread* get_thread_by_tid (struct thread *t, tid_t tid);

/* Starts a new thread running a user program loaded from
   FILENAME.  The new thread may be scheduled (and may even exit)
   before process_execute() returns.  Returns the new process's
   thread id, or TID_ERROR if the thread cannot be created. */
tid_t
process_execute (const char *file_name) 
{
  char *fn_copy;
  tid_t tid;

  /* Make a copy of FILE_NAME.
     Otherwise there's a race between the caller and load(). */
  fn_copy = palloc_get_page (0);
  if (fn_copy == NULL)
    {
      palloc_free_page (fn_copy);
      return TID_ERROR;
    }
    
  strlcpy (fn_copy, file_name, PGSIZE);

  /* Create a new thread to execute FILE_NAME. */
  tid = thread_create (file_name, PRI_DEFAULT, start_process, fn_copy);
  
  /* If the tid is an error, then free page and then return. */
  if (tid == TID_ERROR)
    {
      palloc_free_page (fn_copy); 
      return tid;
    }
    
  /* Get the child thread through the TID. */
  struct thread *child_t = get_thread_by_tid (thread_current (), tid);

  if (child_t)
    {
      /* Wait for child to load. */
      sema_down (&child_t->launching);
    }
  else 
    {
      /* This means child_t is null. */
      tid = TID_ERROR;
      return tid;
    }

  if (child_t->load_bit == -1)
    {
      list_remove (&get_thread_by_tid (thread_current (), tid) -> child);
      tid = TID_ERROR;
    }

  return tid;
}

/* A thread function that loads a user process and starts it
   running. */
static void
start_process (void *file_name_)
{
  char *file_name = file_name_;
  struct intr_frame if_;
  bool success;
  
  /* Initialize interrupt frame and load executable. */
  memset (&if_, 0, sizeof if_);
  if_.gs = if_.fs = if_.es = if_.ds = if_.ss = SEL_UDSEG;
  if_.cs = SEL_UCSEG;
  if_.eflags = FLAG_IF | FLAG_MBS;
  lock_acquire (&sys_lock);
  success = load (file_name, &if_.eip, &if_.esp);
  lock_release (&sys_lock);

  /* If load failed, quit. */
  palloc_free_page (file_name);
  
  struct thread *t = thread_current ();
  if (!success)
    {
      t->load_bit = -1;
      sema_up (&t->launching);
      thread_exit ();
    }
  else
    {
      t->load_bit = 1;
      sema_up (&t->launching);
    }

  /* Start the user process by simulating a return from an
     interrupt, implemented by intr_exit (in
     threads/intr-stubs.S).  Because intr_exit takes all of its
     arguments on the stack in the form of a `struct intr_frame',
     we just point the stack pointer (%esp) to the stack frame
     and jump to it. */
  asm volatile ("movl %0, %%esp; jmp intr_exit" : : "g" (&if_) : "memory");
  NOT_REACHED ();
}

/* Waits for thread TID to die and returns its exit status.  If
   it was terminated by the kernel (i.e. killed due to an
   exception), returns -1.  If TID is invalid or if it was not a
   child of the calling process, or if process_wait() has already
   been successfully called for the given TID, returns -1
   immediately, without waiting.

   This function will be implemented in problem 2-2.  For now, it
   does nothing. */
int
process_wait (tid_t child_tid UNUSED) 
{
  int status = -1;
  if (child_tid == TID_ERROR)
    return status;
  /* Get current thread. */
  struct thread *t = thread_current ();
  /* Get child thread through TID. */
  struct thread *child = get_thread_by_tid (t, child_tid);
  /* If child is null, then we can end now. */
  if (child == NULL)
    {
      return status;
    }
    
  /* This means that the process has already been called. */
  if (child->status_bit == 1)
    return status;
  /* Remove the child from the parent's list. */
  list_remove (&child->child);
  sema_down (&child->dying);
  status = child->exit_status;
  child->status_bit = 1;
  sema_up (&child->dead);
  
  return status;
}

/* Free the current process's resources. */
void
process_exit (void)
{
  struct thread *t = thread_current ();
  sema_up (&t->dying);
  /* Acquire lock to close executeable. */
  if (!lock_held_by_current_thread (&sys_lock))
    {
      lock_acquire (&sys_lock);
    }
    
  file_close (t->e_file);
  lock_release (&sys_lock);

  struct list_elem *e;
  for (e = list_begin (&t->child_list); e != list_end (&t->child_list); 
       e = list_next (e))
    {
      struct thread *child = list_entry (e, struct thread, child);
      /* Sema up each child and remove list element from parent list. */
      sema_up (&child->dead);
      list_remove (e);
    }
    
  /* Close all files. */
  struct file **fd_table = t->fdt;
  int i;
  for (i = 2; i < MAX_ARGS; i++)
    {
      struct file *file = fd_table [i];
      if (file != NULL)
        {
          close (i);
        }
    }

  if (lock_held_by_current_thread (&sys_lock))
    lock_release (&sys_lock);
  
  sema_down (&t->dead);

  struct hash_iterator iter;
  struct hash *supp_table = &t->supp_page_table;
  hash_first (&iter, supp_table);
  while (hash_next (&iter))
    {
      struct page *p = hash_entry (hash_cur (&iter), struct page, supp_elem);
      
      /* check location */
      if (p->loc == SWAP_TABLE)
        {
          /* flip the bit for the swap table */
        }
      else if (p->loc == FRAME_TABLE)
        {
          remove_ft (p);
        }
      /* free (p); */
    }
  hash_destroy (supp_table, NULL); 

  uint32_t *pd;

  /* Destroy the current process's page directory and switch back
     to the kernel-only page directory. */
  pd = t->pagedir;
  if (pd != NULL) 
    {
      /* Correct ordering here is crucial.  We must set
         cur->pagedir to NULL before switching page directories,
         so that a timer interrupt can't switch back to the
         process page directory.  We must activate the base page
         directory before destroying the process's page
         directory, or the active page directory will be one
         that's been freed (and cleared). */
      t->pagedir = NULL;
      pagedir_activate (NULL);
      pagedir_destroy (pd);
    }
}

/* Sets up the CPU for running user code in the current
   thread.
   This function is called on every context switch. */
void
process_activate (void)
{
  struct thread *t = thread_current ();

  /* Activate thread's page tables. */
  pagedir_activate (t->pagedir);

  /* Set thread's kernel stack for use in processing
     interrupts. */
  tss_update ();
}

/* We load ELF binaries.  The following definitions are taken
   from the ELF specification, [ELF1], more-or-less verbatim.  */

/* ELF types.  See [ELF1] 1-2. */
typedef uint32_t Elf32_Word, Elf32_Addr, Elf32_Off;
typedef uint16_t Elf32_Half;

/* For use with ELF types in printf(). */
#define PE32Wx PRIx32   /* Print Elf32_Word in hexadecimal. */
#define PE32Ax PRIx32   /* Print Elf32_Addr in hexadecimal. */
#define PE32Ox PRIx32   /* Print Elf32_Off in hexadecimal. */
#define PE32Hx PRIx16   /* Print Elf32_Half in hexadecimal. */

/* Executable header.  See [ELF1] 1-4 to 1-8.
   This appears at the very beginning of an ELF binary. */
struct Elf32_Ehdr
  {
    unsigned char e_ident[16];
    Elf32_Half    e_type;
    Elf32_Half    e_machine;
    Elf32_Word    e_version;
    Elf32_Addr    e_entry;
    Elf32_Off     e_phoff;
    Elf32_Off     e_shoff;
    Elf32_Word    e_flags;
    Elf32_Half    e_ehsize;
    Elf32_Half    e_phentsize;
    Elf32_Half    e_phnum;
    Elf32_Half    e_shentsize;
    Elf32_Half    e_shnum;
    Elf32_Half    e_shstrndx;
  };

/* Program header.  See [ELF1] 2-2 to 2-4.
   There are e_phnum of these, starting at file offset e_phoff
   (see [ELF1] 1-6). */
struct Elf32_Phdr
  {
    Elf32_Word p_type;
    Elf32_Off  p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
  };

/* Values for p_type.  See [ELF1] 2-3. */
#define PT_NULL    0            /* Ignore. */
#define PT_LOAD    1            /* Loadable segment. */
#define PT_DYNAMIC 2            /* Dynamic linking info. */
#define PT_INTERP  3            /* Name of dynamic loader. */
#define PT_NOTE    4            /* Auxiliary info. */
#define PT_SHLIB   5            /* Reserved. */
#define PT_PHDR    6            /* Program header table. */
#define PT_STACK   0x6474e551   /* Stack segment. */

/* Flags for p_flags.  See [ELF3] 2-3 and 2-4. */
#define PF_X 1          /* Executable. */
#define PF_W 2          /* Writable. */
#define PF_R 4          /* Readable. */

static bool setup_stack (void **esp, char *file_name);
static bool validate_segment (const struct Elf32_Phdr *, struct file *);
static bool load_segment (struct file *file, off_t ofs, uint8_t *upage,
                          uint32_t read_bytes, uint32_t zero_bytes,
                          bool writable);

/* Loads an ELF executable from FILE_NAME into the current thread.
   Stores the executable's entry point into *EIP
   and its initial stack pointer into *ESP.
   Returns true if successful, false otherwise. */
bool
load (const char *file_name, void (**eip) (void), void **esp) 
{
  struct thread *t = thread_current ();
  struct Elf32_Ehdr ehdr;
  struct file *file = NULL;
  off_t file_ofs;
  bool success = false;
  int i;

  /* Allocate and activate page directory. */
  t->pagedir = pagedir_create ();
  if (t->pagedir == NULL) 
    goto done;
  process_activate ();

  supp_page_table_init (&t->supp_page_table); 

  char *str_copy = (char *) palloc_get_page (001 | 002);
  if (str_copy)
    strlcpy (str_copy, file_name, PGSIZE);
  
  /* Extract command name from file. */
  char *save;
  char *command_name = strtok_r (str_copy, " ", &save);
  file = filesys_open (command_name);
  t->e_file = file;
  
  if (file == NULL) 
    {
      printf ("load: %s: open failed\n", command_name);
      goto done; 
    }

  /* Read and verify executable header. */
  if (file_read (file, &ehdr, sizeof ehdr) != sizeof ehdr
      || memcmp (ehdr.e_ident, "\177ELF\1\1\1", 7)
      || ehdr.e_type != 2
      || ehdr.e_machine != 3
      || ehdr.e_version != 1
      || ehdr.e_phentsize != sizeof (struct Elf32_Phdr)
      || ehdr.e_phnum > 1024) 
    {
      printf ("load: %s: error loading executable\n", file_name);
      goto done; 
    }

  /* Read program headers. */
  file_ofs = ehdr.e_phoff;
  for (i = 0; i < ehdr.e_phnum; i++) 
    {
      struct Elf32_Phdr phdr;

      if (file_ofs < 0 || file_ofs > file_length (file))
        goto done;
      file_seek (file, file_ofs);

      if (file_read (file, &phdr, sizeof phdr) != sizeof phdr)
        goto done;
      file_ofs += sizeof phdr;
      switch (phdr.p_type) 
        {
        case PT_NULL:
        case PT_NOTE:
        case PT_PHDR:
        case PT_STACK:
        default:
          /* Ignore this segment. */
          break;
        case PT_DYNAMIC:
        case PT_INTERP:
        case PT_SHLIB:
          goto done;
        case PT_LOAD:
          if (validate_segment (&phdr, file)) 
            {
              bool writable = (phdr.p_flags & PF_W) != 0;
              uint32_t file_page = phdr.p_offset & ~PGMASK;
              uint32_t mem_page = phdr.p_vaddr & ~PGMASK;
              uint32_t page_offset = phdr.p_vaddr & PGMASK;
              uint32_t read_bytes, zero_bytes;
              if (phdr.p_filesz > 0)
                {
                  /* Normal segment.
                     Read initial part from disk and zero the rest. */
                  read_bytes = page_offset + phdr.p_filesz;
                  zero_bytes = (ROUND_UP (page_offset + phdr.p_memsz, PGSIZE)
                                - read_bytes);
                }
              else 
                {
                  /* Entirely zero.
                     Don't read anything from disk. */
                  read_bytes = 0;
                  zero_bytes = ROUND_UP (page_offset + phdr.p_memsz, PGSIZE);
                }
              if (!lazy_load (file, file_page, (void *) mem_page,
                              read_bytes, zero_bytes, writable))
                goto done;
            }
          else
            goto done;
          break;
        }
    }

  /* Set up stack. */
  if (!setup_stack (esp, file_name))
    goto done;
  /* Start address. */
  *eip = (void (*) (void)) ehdr.e_entry;
  file_deny_write (file);
  success = true;

 done:
  /* We arrive here whether the load is successful or not. */
  palloc_free_page (str_copy);
  return success;
}


/* Checks whether PHDR describes a valid, loadable segment in
   FILE and returns true if so, false otherwise. */
static bool
validate_segment (const struct Elf32_Phdr *phdr, struct file *file) 
{
  /* p_offset and p_vaddr must have the same page offset. */
  if ((phdr->p_offset & PGMASK) != (phdr->p_vaddr & PGMASK)) 
    return false; 

  /* p_offset must point within FILE. */
  if (phdr->p_offset > (Elf32_Off) file_length (file)) 
    return false;

  /* p_memsz must be at least as big as p_filesz. */
  if (phdr->p_memsz < phdr->p_filesz) 
    return false; 

  /* The segment must not be empty. */
  if (phdr->p_memsz == 0)
    return false;
  
  /* The virtual memory region must both start and end within the
     user address space range. */
  if (!is_user_vaddr ((void *) phdr->p_vaddr))
    return false;
  if (!is_user_vaddr ((void *) (phdr->p_vaddr + phdr->p_memsz)))
    return false;

  /* The region cannot "wrap around" across the kernel virtual
     address space. */
  if (phdr->p_vaddr + phdr->p_memsz < phdr->p_vaddr)
    return false;

  /* Disallow mapping page 0.
     Not only is it a bad idea to map page 0, but if we allowed
     it then user code that passed a null pointer to system calls
     could quite likely panic the kernel by way of null pointer
     assertions in memcpy(), etc. */
  if (phdr->p_vaddr < PGSIZE)
    return false;

  /* It's okay. */
  return true;
}

/* Loads a segment starting at offset OFS in FILE at address
   UPAGE.  In total, READ_BYTES + ZERO_BYTES bytes of virtual
   memory are initialized, as follows:

        - READ_BYTES bytes at UPAGE must be read from FILE
          starting at offset OFS.

        - ZERO_BYTES bytes at UPAGE + READ_BYTES must be zeroed.

   The pages initialized by this function must be writable by the
   user process if WRITABLE is true, read-only otherwise.

   Return true if successful, false if a memory allocation error
   or disk read error occurs. */
static bool
load_segment (struct file *file, off_t ofs, uint8_t *upage,
              uint32_t read_bytes, uint32_t zero_bytes, bool writable) 
{
  ASSERT ((read_bytes + zero_bytes) % PGSIZE == 0);
  ASSERT (pg_ofs (upage) == 0);
  ASSERT (ofs % PGSIZE == 0);

  file_seek (file, ofs);
  while (read_bytes > 0 || zero_bytes > 0) 
    {
      /* Calculate how to fill this page.
         We will read PAGE_READ_BYTES bytes from FILE
         and zero the final PAGE_ZERO_BYTES bytes. */
      size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
      size_t page_zero_bytes = PGSIZE - page_read_bytes;

      /* Get a page of memory. */
      uint8_t *kpage = (uint8_t*) insert_ft (PAL_USER);
      if (kpage == NULL)
        return false;

      /* Load this page. */
      if (file_read (file, kpage, page_read_bytes) != (int) page_read_bytes)
        {
          /* remove_ft (kpage); */
          return false; 
        }
      memset (kpage + page_read_bytes, 0, page_zero_bytes);

      /* Add the page to the process's address space. */
      if (!install_page (upage, kpage, writable))
        {
          return false; 
        }
      
      /* Create the page struct. */
      struct page *p;
      p = (struct page *) malloc (sizeof (struct page));
      page_init (p, upage, FILE_SYS, writable, page_read_bytes, page_zero_bytes, ofs, 
        file, -1); 

      /* Insert into the supplemental page table. */
      insert_spt (p);

      /* Map the page to a frame. */ 
      map_ptof ((void *) kpage, p);

      /* Advance. */
      read_bytes -= page_read_bytes;
      zero_bytes -= page_zero_bytes;
      upage += PGSIZE;
      ofs += page_read_bytes;
    }
  return true;
}

bool
lazy_load (struct file *file, off_t ofs, uint8_t *upage,
              uint32_t read_bytes, uint32_t zero_bytes, bool writable) 
{
  /*printf ("\n\nLAZY LOADING\n\n");*/
  ASSERT ((read_bytes + zero_bytes) % PGSIZE == 0);
  ASSERT (pg_ofs (upage) == 0);
  ASSERT (ofs % PGSIZE == 0);

  file_seek (file, ofs);
  while (read_bytes > 0 || zero_bytes > 0) 
    {
      size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
      size_t page_zero_bytes = PGSIZE - page_read_bytes;
      
      /* Create the page struct. */
      struct page *p;
      p = (struct page *) malloc (sizeof (struct page));
      page_init (p, upage, FILE_SYS, writable, page_read_bytes, page_zero_bytes, ofs, 
        file, -1); 

      /* Insert into the supplemental page table. */
      
      bool result = insert_spt (p);
      
      if (!result)
        return false;
      
      /* Advance. */
      read_bytes -= page_read_bytes;
      zero_bytes -= page_zero_bytes;
      upage += PGSIZE;
      ofs += page_read_bytes;
    }
  return true;
}

/* Create a minimal stack by mapping a zeroed page at the top of
   user virtual memory. */
static bool
setup_stack (void **esp, char *file_name) 
{
  uint8_t *kpage;
  bool success = false;
  
  /* Create the page struct. */
  struct page *p;
  p = (struct page *) malloc (sizeof (struct page));
  page_init (p, ((uint8_t *) PHYS_BASE) - PGSIZE, FILE_SYS, true, 0, PGSIZE, 0, NULL, -1); 

  /* Insert into the supplemental page table. */
  bool result = insert_spt (p);
  
  if (!result)
    return false;
  
  kpage = (uint8_t*) insert_ft (PAL_USER | PAL_ZERO);
  if (kpage != NULL) 
    {
      
      if (install_page (p->p_ptr, kpage, true) && load_page (kpage, p))
        *esp = PHYS_BASE;
      else
        {
          remove_ft (p);
          return false;
        }
    } 
  else 
    {
      PANIC ("No frames left.");
     
      return false;
    }
  

  
  

  /* Create array to store arguments. */
  char **argv = (char**) palloc_get_page (001 | 002);
  char *save_ptr;
  int argc = 0;
  /* If palloc failed, return status. */
  if (!argv)
    {
      palloc_free_page (argv);
      return false;
    }

  char *token;
  
  for (token = strtok_r (file_name, " ", &save_ptr); token != NULL;
       token = strtok_r (NULL, " ", &save_ptr))
    {
      argv [argc] = token;
      argc++;
      
      if (argc > MAX_ARGS)
        {
          palloc_free_page (argv);
          return false;
        }
    }

  char *myESP = (char *) *esp;
  char *adr [argc];
  int adr_len = 0;
  int num_args = argc - 1;
  int align = 0;

  /* Push arguments on to stack. */
  char *arg = argv [num_args];
  while (num_args >= 0) 
  {
    int str_len = strlen (arg) + 1;
    if (space_on_stack (myESP, str_len))
      {
        
        myESP -= str_len;
        align += str_len;

        strlcpy (myESP, arg, str_len);

        adr [adr_len] = myESP;
        adr_len++;
        
        num_args--;
        arg = argv [num_args];
      }
    else
      {
        palloc_free_page (argv);
        return false;
      }
  }

  int offset = 4 - (align % 4);
  if (space_on_stack (myESP, offset + sizeof (char*)))
    {
      if (offset != 4)
        {
          myESP -= offset;
        }
     
      myESP -= 4;
    }
  else
    {
      palloc_free_page (argv);
      return false;
    }
  
  int counter = 0;  
  while (counter < adr_len) 
    {
      if (space_on_stack (myESP, sizeof (char*)))
        {
          myESP -= sizeof (char*);
          memcpy (myESP, &adr [counter], sizeof (char*));
          counter++;
        }
      else
        {
          palloc_free_page (argv);
          return false;
        }
    }

  if (space_on_stack (myESP, sizeof (char*) * 2 + sizeof (int)))
    {
      char *argv_adr = myESP;
      myESP -= sizeof (char**);
      memcpy (myESP, &argv_adr, sizeof (char**));
      
        myESP -= sizeof (int);
        memcpy (myESP, &argc, sizeof (int));
    
        myESP -= sizeof (char*);
    }
  else
    {
      palloc_free_page (argv);
      return false;
    }

  *esp = myESP;
  
  palloc_free_page (argv);
  return true;
}

static bool
space_on_stack (char *esp, int arg_len)
{
  char *tempESP = esp;
  if (!tempESP)
    {
      return false;
    }
  
  int bit;
  for (bit = 0; bit < arg_len; bit++)
    {
      if (tempESP - 1 < (PHYS_BASE - PGSIZE)) 
        {
          return false;
        } 
      tempESP--;
    }
  return true;
}

/* Adds a mapping from user virtual address UPAGE to kernel
   virtual address KPAGE to the page table.
   If WRITABLE is true, the user process may modify the page;
   otherwise, it is read-only.
   UPAGE must not already be mapped.
   KPAGE should probably be a page obtained from the user pool
   with palloc_get_page().
   Returns true on success, false if UPAGE is already mapped or
   if memory allocation fails. */
bool
install_page (void *upage, void *kpage, bool writable)
{
  struct thread *t = thread_current ();
   
  return (pagedir_get_page (t->pagedir, upage) == NULL
          && pagedir_set_page (t->pagedir, upage, kpage, writable));
}

static struct thread*
get_thread_by_tid (struct thread* t, tid_t tid)
{
    struct thread *tid_child = NULL;
    struct list_elem *e;
    int done = 0;

    for (e = list_begin (&t->child_list); 
       e != list_end (&t->child_list) && !done; 
       e = list_next (e))
      {
        struct thread *child = list_entry (e, struct thread, child);
        if (child->tid == tid)
          {
            tid_child = child;
            done = 1;
          }
      }
    return tid_child;
}
