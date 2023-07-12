#ifndef VM_SWAP_H
#define VM_SWAP_H

#include "vm/page.h"
#include "userprog/syscall.h"
#include <stdio.h>
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "threads/synch.h"
#include "filesys/file.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "vm/frame.h"
#include "threads/synch.h"
#include "devices/block.h"
#include "lib/kernel/bitmap.h"

struct bitmap *swap_table;

void swap_table_init (void);
bool swap_entry_insert (struct page *p);
void swap_entry_remove (void *f_ptr, struct page *p);


#endif /* vm/swap.h */