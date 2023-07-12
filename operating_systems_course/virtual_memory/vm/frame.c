#include "vm/frame.h"

int MAX_STACK = 2000 * PGSIZE;
int FAULT_ADDR_LIMIT = 32;



/* Initializes a frame struct */
void
frame_init (struct frame *f, void *f_ptr)
{
    f->frame_ptr = f_ptr;
    f->t = thread_current ();
}

void
map_ptof (void *f_ptr, struct page *p_ptr)
{
    struct frame *f = lookup_frame (f_ptr);
    if (f)
        {
            f->page_ptr = p_ptr;
            p_ptr->frame_ptr = f;
        }

}

struct frame *
lookup_frame (void *f_ptr)
{
    lock_acquire (&frame_lock);
    struct list_elem *e;
            
    for (e = list_begin (&frame_table); e != list_end (&frame_table);
        e = list_next (e))
        {
            struct frame *f = list_entry (e, struct frame, elem);
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
void frame_table_init(void)
{
    /* Initialize a linked list. */
    list_init (&frame_table);
    lock_init (&frame_lock);
    clock_pointer = NULL;
}

void *
insert_ft (enum palloc_flags flags)
{
    void* f_ptr = palloc_get_page (flags);

    if (!f_ptr)
    {
      /* Call eviction policy */
      evict ();
      f_ptr = palloc_get_page (flags);
    }

    /* Make the frame struct. */
    struct frame *f;
    f = (struct frame *) malloc (sizeof (struct frame));
    if (f) 
        {
            frame_init (f, f_ptr);
        }
    lock_acquire (&frame_lock);
    /* Add frame to the table. */
    list_push_back (&frame_table, &f->elem);
    lock_release (&frame_lock);
    /* Increment size of table. */
    frame_table_size ++;

   return f_ptr;
}

/* Removing from the frame table. */
void
remove_ft (struct page *page)
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
                    struct frame *f = list_entry (e, struct frame, elem);
                    if (f->page_ptr == page)
                        {
                            if (!lock_held_by_current_thread (&frame_lock))
                                lock_acquire (&frame_lock);
                            pagedir_clear_page (f->t->pagedir, page->p_ptr);
                            list_remove (&(f->elem));
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
    p = lookup (&t->supp_page_table, 
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
                
                /* Insert into the supplemental page table. */
                bool result = insert_spt (p);

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
                elem);
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
                        }
                    else
                        {
                           /* Page is not dirty, send to filesys. Change location of page
                                to filesys. */ 
                            p->loc = FILE_SYS;
                        }    
                    lock_acquire (&frame_lock);
                    /* Remove entry from the frame table. */
                    clock_pointer = list_remove (clock_pointer);
                    frame_table_size --;
                    palloc_free_page (f->frame_ptr);
                    free (f);
                    lock_release (&frame_lock);
                    lock_release (&p->pin);
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
            struct frame *f = list_entry (e, struct frame, elem);
            printf ("The frame's thread name is %d\n", f->t->tid);

            i++;
        }
        printf("\n\n");
}
