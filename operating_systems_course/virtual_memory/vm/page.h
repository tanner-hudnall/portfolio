#ifndef VM_PAGE_H
#define VM_PAGE_H


#include <hash.h>
#include "lib/kernel/hash.h"
#include <stdint.h>
#include <inttypes.h>
#include "userprog/process.h"
#include <debug.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "userprog/gdt.h"
#include "userprog/pagedir.h"
#include "userprog/tss.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/flags.h"
#include "threads/init.h"
#include "threads/synch.h"
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/syscall.h"
#include "vm/frame.h"
#include "vm/swap.h"
/* Location of where the page is. */
enum location
    {
        FILE_SYS,          /* In file system. */
        SWAP_TABLE,        /* In swap. */
        FRAME_TABLE,       /* In frame table. */                
    };

struct page 
    {
      enum location loc;
      void *p_ptr;
      struct hash_elem supp_elem;
      bool is_writable;
      off_t offset;
      uint32_t read_bytes;
      uint32_t zero_bytes;
      struct file *file;
      size_t sector_index;
      struct frame *frame_ptr;
      bool is_loaded;
      struct lock pin;
    };

void page_init (struct page *p, void *p_ptr, enum location loc, bool writable, uint32_t read_bytes, uint32_t zero_bytes, off_t offset, struct file *file, block_sector_t sector_index);
void supp_page_table_init (struct hash *table);
unsigned page_hash (const struct hash_elem *p_, void *aux UNUSED); 
bool load_page (void *f_ptr, struct page *p);
void page_destructor (struct hash_elem *hash_elem, void *aux UNUSED);
bool page_order (const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED);
struct page *page_lookup (struct hash *table, const void *address); 
bool insert_spt (struct page *p);


#endif /* vm/page.h */
