#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "threads/palloc.h"
#include "vm/page.h"
#include <stdint.h>
#include <hash.h>
#include "lib/kernel/hash.h"

struct frame 
{
    void *frame_ptr;
    struct page *page_ptr;
    struct list_elem table_elem;
    struct thread *t; /* Thread owning the process. */
    /* Get frame number and offset? */
};

struct list frame_table; /* Global frame table. */
struct lock frame_lock; 
struct list_elem *clock_pointer;
int frame_table_size;
void frame_init (struct frame *f, void *f_ptr);
void map_ptof (void *f_ptr, struct page *p_ptr);
struct frame * f_lookup (void *f_ptr);
void frame_table_init (void); /* Initializes our frame table list. */
void *frame_table_insert (enum palloc_flags flags);
void frame_table_remove (struct page *page);
struct page* stack_growth (void *upage, struct intr_frame *f);
bool evict (void);


#endif /* vm/frame.h */
