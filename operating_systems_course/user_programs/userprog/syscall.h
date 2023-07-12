#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init(void);

bool valid_ptr(void *addr);

void exit(int status);
#endif /* userprog/syscall.h */

