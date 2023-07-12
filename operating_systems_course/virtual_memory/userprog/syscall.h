#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "devices/shutdown.h"
#include "threads/synch.h"
#include "filesys/file.h"
#include "vm/frame.h"
#include "threads/malloc.h"
#include "userprog/exception.h"
#include "vm/page.h"

/* Maximum number of arguments in command line. */
#define MAX_ARGS 128
/*#define MAX_STACK_SIZE = 2000 * PGSIZE */                        

/* Declare a file sys lock. */
struct lock sys_lock;

void syscall_init (void);
void close (int fd);
void exit (int status);

#endif /* userprog/syscall.h */
