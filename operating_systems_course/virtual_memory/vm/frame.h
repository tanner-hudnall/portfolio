#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "threads/palloc.h"
#include "vm/page.h"
#include <stdint.h>
#include <hash.h>
#include "lib/kernel/hash.h"
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
#include "threads/synch.h"
#include "lib/kernel/hash.h"
#include <hash.h>
#include <list.h>
#include "userprog/exception.h"
#include <devices/block.h>
#include "vm/swap.h"

struct frame 
{
    void *frame_ptr;
    struct page *page_ptr;
    struct list_elem elem;
    struct thread *t; 
};

struct list frame_table; /* Global frame table. */
struct lock frame_lock; 
struct list_elem *clock_pointer;
int frame_table_size;
void frame_init (struct frame *f, void *f_ptr);
void map_ptof (void *f_ptr, struct page *p_ptr);
struct frame * lookup_frame (void *f_ptr);
void frame_table_init(void); 
void *insert_ft (enum palloc_flags flags);
void remove_ft(struct page *page);
struct page* stack_growth (void *upage, struct intr_frame *f);
bool evict (void);
static void print_frames (void);


#endif /* vm/frame.h */
