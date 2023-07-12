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
#include "lib/kernel/hash.h"
#include <hash.h>
#include <list.h>
#include "userprog/exception.h"
#include <devices/block.h>
#include "vm/swap.h"

int MAX_STACK = 2000 * PGSIZE;
int FAULT_ADDR_LIMIT = 32;

static void print_frames (void);

/* Initializes a frame struct */
void
frame_init (struct frame *f, void *f_ptr)
{
    f->frame_ptr = f_ptr;
    f->t = thread_current ();
    /* LIST OF PAGE POINTERS ?????? HANDLE ALIASES?????? */
}

void
map_ptof (void *f_ptr, struct page *p_ptr)
{
    struct frame *f = f_lookup (f_ptr);
    if (f)
        {
            f->page_ptr = p_ptr;
            p_ptr->frame_ptr = f;
        }
    /* shit should not happen - 
    we return false if kpage is null in load segment. */
}

struct frame *
f_lookup (void *f_ptr)
{
    lock_acquire (&frame_lock);
    struct list_elem *e;
            
    for (e = list_begin (&frame_table); e != list_end (&frame_table);
        e = list_next (e))
        {
            struct frame *f = list_entry (e, struct frame, table_elem);
            if (f_ptr == f->frame_ptr)
                {
                    lock_release (&frame_lock);
                    return f;
                }
        }
    lock_release (&frame_lock);
    return NULL;
}

/* Initialized the frame table. */
void 
frame_table_init (void)
{
    /* Initialize a linked list. */
    list_init (&frame_table);
    lock_init (&frame_lock);
    clock_pointer = NULL;
}

void *
frame_table_insert (enum palloc_flags flags)
{
    /* printf ("\n\nFrame Table Size: %d", frame_table_size); */
    void* f_ptr = palloc_get_page (flags);

    if (!f_ptr)
    {
      /* Call eviction policy */
      evict ();
      f_ptr = palloc_get_page (flags);
      /* printf ("\n\nGOT PAGE AFER EVICT!!\n\n"); */
    }

    /* Make the frame struct. */
    struct frame *f;
    f = (struct frame *) malloc (sizeof (struct frame));
    /* printf ("\n\nMALLOC NEW FRAME AFTER EVICT!!\n\n"); */
    if (f) /* WTF TO DO IF MALLOC FAILS? */
        {
            frame_init (f, f_ptr);
        }
    lock_acquire (&frame_lock);
    /* Add frame to the table. */
    list_push_back (&frame_table, &f->table_elem);
    lock_release (&frame_lock);
    /* Increment size of table. */
    frame_table_size ++;

    /* printf ("\n\nFRAME TABLE INSERT FINISHED!!\n\n"); */
        
    /*else
        {
            PANIC("FAILED TO GET PAGE \n");
        }*/
    /* 
    We check if the current size is the limit (number of user pages)
    EVERYTIME WE CONVERT A VIRTUAL ADDRESS TO PHYSICAL ADDRESS
    WE NEED TO CHECK EXISTING FRAME STRUCTS AND THE PHYSICAL ADDRESS
    TO SEE IF THE PHYSICAL ADDRESS WE WANT TO ALLOCATE FOR MATCHES
    ANY OF THE EXISTING SLOTS
    if (size == get_user_pages ())
        {
            // go through swap and see if we have an empty space - helper
                in this, we need to remove from the list, put in swap, 
                decrement the size, and then continue on with the rest of the
                function
            // if we can't evict in swap, then panic kernel
        }

    once we add, we need to increment the table size
    */
   return f_ptr;
}

/* Removing from the frame table. */
void
frame_table_remove (struct page *page)
{
    if (page)
        {
            if (!lock_held_by_current_thread (&page->pin))
                lock_acquire (&page->pin);
            struct list_elem *e;
            int done = 0;
            
            for (e = list_begin (&frame_table);
                 e != list_end (&frame_table) && !done;
                 e = list_next (e))
                {
                    struct frame *f = list_entry (e, struct frame, table_elem);
                    if (f->page_ptr == page)
                        {
                            if (!lock_held_by_current_thread (&frame_lock))
                                lock_acquire (&frame_lock);
                            pagedir_clear_page (f->t->pagedir, page->p_ptr);
                            list_remove (&(f->table_elem));
                            palloc_free_page (f->frame_ptr);
                            free (f); 
                            frame_table_size --;
                            done = 1;
                            lock_release (&frame_lock);
                        }
                }
            lock_release (&page->pin);
        }

}

/* Function for stack growth. */
struct page *
stack_growth (void *upage, struct intr_frame *f)
{
    struct page *p;
    struct thread *t = thread_current ();
    /* Look for the faulting page in the supp table. */
    p = page_lookup (&t->supp_page_table, 
                     pg_round_down (upage));

    /* Check if page is null. */
    if (!p)
        {
          /* Stack growth. */
          if (upage < PHYS_BASE && upage >= PHYS_BASE - MAX_STACK
              && (upage >= f->esp - FAULT_ADDR_LIMIT))
              {
                /* Create the page struct. */
                p = (struct page *) malloc (sizeof (struct page));
                page_init (p, pg_round_down (upage), FILE_SYS, true,
                          0, PGSIZE, 0, NULL, -1); 
                
                /* Insert into our supplemental page table. */
                bool result = supp_page_table_insert (p);

                if (!result)
                    return NULL;
              }
          else 
              return NULL;
        }
    return p;
}

/* Eviction policy function. */
bool
evict (void)
{
    bool evicted = false;
    struct frame *f;
    struct page *p;
    struct thread *t;
    /* If running policy for first time, then initialize ptr to first elem. */
    if (clock_pointer == NULL)
        clock_pointer = list_begin (&frame_table);
    int i = 0;
    while (!evicted)
        {
            f = list_entry (clock_pointer, struct frame, 
                table_elem);
            p = f->page_ptr;
            t = f->t;

            if (pagedir_is_accessed (t->pagedir, p->p_ptr))
                {
                    /* Set to 0. */
                    pagedir_set_accessed (t->pagedir, p->p_ptr, 
                                          false);
                    /* printf ("\n\nSET TO 0\n\n"); */
                }
            else if (!lock_held_by_current_thread (&p->pin) && lock_try_acquire (&p->pin))
                {
                    /*
                    try to acquire lock, if cannot, continue otherwise acquire lock and do everything we need
                    */
                    /* Evict. */
                    evicted = true;
                    /* Put page in swap or just filesys. */
                    if (pagedir_is_dirty (t->pagedir, p->p_ptr))
                        {
                            swap_entry_insert (p);
                            /*if (!swap_entry_insert (p))
                                PANIC ("FAILED TO SWAP");*/
                            /* rintf ("\n\nINSERT TO SWAP\npage swapped: %p\n\n", p->p_ptr); */
                        }
                    else
                        {
                           /* Page is not dirty, send to filesys. Change location of page
                                to filesys. */ 
                            p->loc = FILE_SYS;
                            /* printf ("\n\nINSERT TO FILESYS\n\n"); */
                        }    
                    lock_acquire (&frame_lock);
                    /* Remove entry from the frame table. */
                    clock_pointer = list_remove (clock_pointer);
                    frame_table_size --;
                    /* pagedir_clear_page (t->pagedir, p->p_ptr); */
                    palloc_free_page (f->frame_ptr);
                    /* printf ("PALLOC FREE PAGE !!!"); */
                    free (f);
                    /* printf ("FREE!!"); */
                    lock_release (&frame_lock);
                    lock_release (&p->pin);
                    /* need to free list_elem?????? */
                }
            if (!evicted)
            {
                if (clock_pointer == NULL || 
                    list_back (&frame_table) == clock_pointer)
                {
                    clock_pointer = list_begin (&frame_table);
                }
                else
                    clock_pointer = list_next (clock_pointer);
            } 
            else 
            {
                
                if (clock_pointer == NULL)
                {
                    clock_pointer = list_begin (&frame_table);
                }
                /* printf("Evicted!!"); */
            }
        }
}

static void
print_frames (void)
{
    struct list_elem *e;
    int i = 0;
    printf("\n\n");
    for (e = list_begin (&frame_table); e != list_end (&frame_table);
        e = list_next (e))
        {
            struct frame *f = list_entry (e, struct frame, table_elem);
            printf ("The frame's thread name is %d\n", f->t->tid);
            /*
            printf("\n\n The frame's page pointer is %p\n\n", f->page_ptr);
            printf("\n\n The frame's frame pointer is %p\n\n", f->frame_ptr);
            printf("\n\n The index of the frame is %d\n\n", i);
            */
            i++;
        }
        printf("\n\n");
}
