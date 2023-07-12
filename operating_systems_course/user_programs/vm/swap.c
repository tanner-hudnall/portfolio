#include "vm/swap.h"
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
#include "vm/frame.h"
#include "threads/synch.h"
#include "devices/block.h"
#include "lib/kernel/bitmap.h"

static struct lock swap_lock;
static struct block *swap_dev;
static int SECTORS_PER_PAGE = PGSIZE / BLOCK_SECTOR_SIZE;

/* Initalize the swap table for the system. */
void 
swap_table_init (void)
{
    swap_dev = block_get_role (BLOCK_SWAP);
    swap_table = bitmap_create (block_size (swap_dev));
    lock_init (&swap_lock);
}

bool
swap_entry_insert (struct page *p)
{
    /*8 sectors is one page*/
    struct frame *f = p->frame_ptr;
    void *phys_addr = f->frame_ptr;

    lock_acquire (&swap_lock);
    size_t index = bitmap_scan_and_flip (swap_table, 0, SECTORS_PER_PAGE, false);
    
    if (index != BITMAP_ERROR)
    {
        p->sector_index = index;
        p->loc = SWAP_TABLE;
        
        int count = 0;
        for (count = 0; count < SECTORS_PER_PAGE; count++)
        {
            block_write (swap_dev, index, phys_addr);
            index++;
            phys_addr += BLOCK_SECTOR_SIZE;
        }
        p->frame_ptr = NULL;
        lock_release (&swap_lock);
        return true;
    }
    lock_release (&swap_lock);
    return false;
}

/* Loading it back from swap into the frame table. */
void
swap_entry_remove (void *f_ptr, struct page *p)
{
    size_t index = p->sector_index;
    void *phys_addr = f_ptr;
    lock_acquire (&swap_lock);
    p->sector_index = -1;
    p->loc = FRAME_TABLE;
    p->frame_ptr = f_lookup (f_ptr); 
    int count = 0;
    for (count = 0; count < SECTORS_PER_PAGE; count++)
        {
            block_read (swap_dev, index, phys_addr);
            index++;
            phys_addr += BLOCK_SECTOR_SIZE;
        }
    pagedir_set_accessed (p->frame_ptr->t->pagedir, p->p_ptr, true);
    bitmap_set_multiple (swap_table, index, SECTORS_PER_PAGE, false);
    lock_release (&swap_lock);
}

void 
swap_table_remove (void)
{
    
}
