#ifndef VM_SWAP_H
#define VM_SWAP_H

#include "vm/page.h"

struct bitmap *swap_table;

void swap_table_init (void);
bool swap_entry_insert (struct page *p);
void swap_entry_remove (void *f_ptr, struct page *p);
void swap_table_remove (void);

#endif /* vm/swap.h */