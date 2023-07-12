#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

/* Maximum number of arguments in command line. */
#define MAX_ARGS 128                       

/* Declare a file sys lock. */
struct lock sys_lock;

void syscall_init (void);
void close (int fd);
void exit (int status);
void debug(const char *fmt, ...);


#endif /* userprog/syscall.h */
