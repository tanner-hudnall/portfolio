#include "userprog/syscall.h"
#include <stdio.h>
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "threads/synch.h"
#include "filesys/file.h"
#include "threads/palloc.h"
#include "vm/page.h"
#include "threads/malloc.h"
#include <hash.h>
#include "lib/kernel/hash.h"

/* Initialize the page struct. */
void 
page_init (struct page *p, void *p_ptr, enum location loc, bool writable,
           uint32_t read_bytes, uint32_t zero_bytes, off_t offset, struct
           file *file, size_t sector_index) 
{
    p->p_ptr = p_ptr;
    p->loc = loc;
    p->is_writable = writable;
    p->read_bytes = read_bytes;
    p->zero_bytes = zero_bytes;
    p->offset = offset;
    p->file = file;
    p->sector_index = sector_index;
    p->frame_ptr = NULL;
    p->is_loaded = true;
    lock_init (&p->pin);
}

/* Initialize the thread's supplemental page table. */
void 
supp_page_table_init (struct hash *table)
{
    hash_init (table, page_hash, page_less, NULL);
}

/* Returns a hash value for page p. */
unsigned
page_hash (const struct hash_elem *p_, void *aux UNUSED)
{
  const struct page *p = hash_entry (p_, struct page, supp_elem);
  return hash_bytes (&p->p_ptr, sizeof p->p_ptr); /* NEED AMPERSAND??? */
}

/* Returns true if page a precedes page b. */
bool
page_less (const struct hash_elem *a_, const struct hash_elem *b_,
           void *aux UNUSED)
{
  const struct page *a = hash_entry (a_, struct page, supp_elem);
  const struct page *b = hash_entry (b_, struct page, supp_elem);

  return a->p_ptr < b->p_ptr;
}

/* Returns the page containing the given virtual address,
   or a null pointer if no such page exists. */
struct page *
page_lookup (struct hash *table, const void *address)
{
  struct page p;
  struct hash_elem *e;

  p.p_ptr = address;
  e = hash_find (table, &p.supp_elem);
  return e != NULL ? hash_entry (e, struct page, supp_elem) : NULL;
}

/* Called in install_page() to insert into our supp page table. */
bool
supp_page_table_insert (struct page *p)
{
    /* Check size of supplemental page table */
    struct hash *supp_table = &(thread_current ()->supp_page_table);
    struct hash_elem *h = hash_insert (supp_table, &p->supp_elem);
    if (h)
      {
        /* This means we already allocated into the page table. */
        return false;
      }
    return true;
}

bool
load_page (void *f_ptr, struct page *p)
{
  /*
  /* Fetch data into the frame based on the location. */
  /* pull from swap, or file read at */
  /* acquire lock*/
  if(!lock_held_by_current_thread (&p->pin))
    lock_acquire (&p->pin);
  if (p->loc == SWAP_TABLE)
    {
      swap_entry_remove (f_ptr, p);
      pagedir_set_dirty (thread_current ()->pagedir, p->p_ptr, false);
      pagedir_set_accessed (thread_current ()->pagedir, p->p_ptr, true);
    }
  else if (p->zero_bytes != PGSIZE && p->loc == FILE_SYS)
    {
      
      /* Load this page. */
      if (file_read_at (p->file, f_ptr, p->read_bytes, p->offset) != (int) p->read_bytes)
        {
          return false; 
        }
      
      memset (f_ptr + p->read_bytes, 0, p->zero_bytes);
    }
  /* printf("\n\nFrame Pointer:%p\n\n", f_ptr);*/
  /*printf("\n\nFault Addrr in load: %p\n\n", p->p_ptr);
  Load zero page. */
  if (p->zero_bytes == PGSIZE)
    {
      memset (f_ptr, 0, PGSIZE);
    } 
  
  /* only change location if true - may need to move this code somewhere else */
   p->loc = FRAME_TABLE;
   lock_release (&p->pin);
   /*release lock */
   return true;
}

void page_destructor (struct hash_elem *hash_elem, void *aux UNUSED)
  {
    struct page *p = hash_entry (hash_elem, struct page, supp_elem);
    free (p);
    remove_elem (&thread_current ()->supp_page_table, hash_elem);
  }