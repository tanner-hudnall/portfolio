#include <stdio.h>
#include <string.h>
#include <syscall-nr.h>
#include "threads/synch.h"
#include "threads/palloc.h"
#include "filesys/filesys.h"
#include "userprog/process.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "process.h"
#include "userprog/syscall.h"
#include "userprog/pagedir.h"

static void syscall_handler(struct intr_frame *);
void halt(void);
void exit(int status);
tid_t exec(const char *cmd_line);
int wait (tid_t pid);
bool create(const char *file, unsigned initial_size);
bool remove(const char *file);
int open(const char *file);
int filesize(int fd);
int read(int fd, void *buffer, unsigned size);
int write(int fd, const void *buffer, unsigned size);
void seek(int fd, unsigned position);
unsigned tell (int fd);
void close(int fd);
struct fileDescriptor * getFileDescriptor(int fd);
void halt(void);

static int get_user(const uint8_t *uaddr);
static bool put_user(uint8_t *udst, uint8_t byte);
static void check_ptr(void* uaddr, size_t size);
static void put_checker(void* uaddr, size_t size);

struct lock fs_lock;


/* Reads a byte at user virtual address UADDR.
   UADDR must be below PHYS_BASE.
   Returns the byte value if successful, -1 if a segfault
   occurred. */
static int get_user (const uint8_t *uaddr)
{
  int result;
  asm ("movl $1f, %0; movzbl %1, %0; 1:"
       : "=&a" (result) : "m" (*uaddr));
  return result;
}
 
 /*Writes BYTE to user address UDST.
   UDST must be below PHYS_BASE.
   Returns true if successful, false if a segfault occurred. */
static bool put_user (uint8_t *udst, uint8_t byte)
{
  int error_code;
  asm ("movl $1f, %0; movb %b2, %1; 1:"
       : "=&a" (error_code), "=m" (*udst) : "q" (byte));
  return error_code != -1;
}

static void check_ptr(void* uaddr, size_t size) {
	if(!valid_ptr(uaddr)) {
		exit(-1);
	}
	for(uint32_t i = 0; i < size; i++) {
		int byte = get_user(uaddr + i);
		if(byte == -1) {
			exit(-1);
		}

	}

}

static void put_checker(void* uaddr, size_t size) {

	if(!valid_ptr(uaddr)) {
		exit(-1);
	}
	for(uint32_t i = 0; i < size; i++) {
		bool isValid = put_user(uaddr + i, sizeof(char));
		if(isValid == false) {
			exit(-1);
		}

	}

}



bool valid_ptr(void *addr) {
	if(addr == NULL) {
		return false;
	}
	else if(!is_user_vaddr(addr)) {
		return false;
	}
	else if(pagedir_get_page(thread_current()->pagedir, addr) == NULL) {
		return false;
	}

	return true;
}


void syscall_init(void)
{
	
    	intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
	lock_init(&fs_lock);
}

//TODO: validate pointers and pass the correct information to each syscall
static void
syscall_handler(struct intr_frame *f UNUSED)
{
   /* Remove these when implementing syscalls */
    int *reference = f->esp;
    uint32_t *returnAddress = &f->eax;


    int callno;
    callno = *reference;
   // we should add valid access functions HERE before even calling, i think that would work better

    switch(callno){
	case SYS_HALT:
	{
		//printf("system call: HALT!\n");
		halt();
		break;
	}
	case SYS_EXIT:
	{
		//printf("system call: EXIT!\n");
		int status = *(reference + 1);
		exit(status);
		break;

	}

	case SYS_EXEC:
	{
		const char *execStatement = (char*) *(reference + 1);
		*returnAddress = (int) exec(execStatement);
		break;
	}

	case SYS_WAIT:
	{
		tid_t pid = *(reference +1);
		*returnAddress = (int) wait(pid);
		check_ptr(reference + 1, sizeof(pid));
		//printf("system call: WAIT!\n");
		break;
	}
	case SYS_CREATE:
	{
		const char *file = (char*) *(reference + 1);
		unsigned size = *(reference + 2);
		check_ptr(reference + 1, sizeof(file));
		check_ptr(reference + 2, sizeof(size));
		*returnAddress = (int) create(file, size);
		//printf("system call: CREATE!\n");
		break;
	}
	case SYS_REMOVE:
	{
		const char *file = (char*) *(reference+1);
		*returnAddress = remove(file);
		//printf("system call: REMOVE!\n");
		break;
	}
	case SYS_OPEN:
	{
		//printf("system call: OPEN!\n");
		const char *file = (char*) *(reference+1);
		check_ptr(reference + 1, sizeof(file));
		*returnAddress = open(file);
		break;
	}
	case SYS_FILESIZE:
	{
		//printf("system call: FILESIZE!\n");
		int fd = *(reference + 1);
		*returnAddress = filesize(fd);
		break;
	}
	case SYS_READ:
	{
		//printf("system call: READ!\n");
		int fd = *(reference + 1);
		check_ptr(reference + 1, sizeof(int));
		void *buffer = (void *) *(reference + 2);
		unsigned size = *(reference + 3);
		put_checker(buffer, size);
		*returnAddress = (int) read(fd, buffer, size);
		break;
	}
	case SYS_WRITE:
	{
		//printf("system call: WRITE!\n");
		int fd = *(reference + 1);
		void *buffer = (void *) *(reference + 2);
		unsigned size = *(reference + 3);
		
		check_ptr(reference+1, sizeof(fd));
		check_ptr(reference+2, sizeof(buffer));
		check_ptr(reference+3, sizeof(size));
		//printf("%d, %d\n",fd, size);

		*returnAddress = (int) write(fd, buffer, size);
		break;
	}
	case SYS_SEEK:
	{
		//printf("system call: SEEK!\n");
		int fd = *(reference+1);
		unsigned position = *(reference + 2);
		check_ptr(reference+1, sizeof(int));
		check_ptr(reference+2, sizeof(unsigned));
		seek(fd, position);
		break;
		//printf("%d, %d\n", fd, position);
	}
	case SYS_TELL:
	{
		//printf("system call: TELL!\n");
		int fd = *(reference+1);
		check_ptr(reference+1, sizeof(int));
		*returnAddress = (int) tell(fd);
		break;
		//printf("%d\n", fd);
	}
	case SYS_CLOSE:
	{
		//printf("system call: CLOSE!\n");
		int fd = *(reference+1);
		check_ptr(reference+1, sizeof(int));
		check_ptr(reference + 4, sizeof(fd));
		close(fd);
		break;

	}
    }

   // thread_exit();
} 

int write(int fd, const void *buffer, unsigned size) {
	//if fd ==1, then write to console
	//else get file from list according to fd
	//if the elemend is null, return -1
	// acquire lock, call file_write, release lock
	//return bytes written (if fd==1, return size, if not return file_write int, or -1 if somehow invalid
	int returnSize = (int) size;
	if(fd == 1){
		putbuf((char*) buffer, size);
		return returnSize;
	}
	else{
		struct fileDescriptor *f = getFileDescriptor(fd);
		if(f == NULL){
			return -1;
		}
		else{
			lock_acquire(&fs_lock);
			returnSize = file_write(f->filePointer, (char*) buffer, size);
			lock_release(&fs_lock);
			return returnSize;
		}
		
	}
}

//should be ready to test
void seek(int fd, unsigned position) {
	//create file, initialize to null
	//look in filedescriptor list, (takes current thread and file descriptor)
	//if nothing is found (NULL POINTER), return
	//else, get lock, file_seek with the file, and position
	//release lock
	struct fileDescriptor *f = getFileDescriptor(fd);
	if(f == NULL){
		return;
	}
	else{
		struct file *seeker = f->filePointer;
		lock_acquire(&fs_lock);
		file_seek(seeker, position);
		lock_release(&fs_lock);
	
	}
	

}
//should be ready to test
unsigned tell(int fd) {
	//set file to null
	//get the file from the list traverser
	//if its null, return 0
	//else get lock, call file_tell, release lock
	//return position
	unsigned position;
	struct fileDescriptor *f = getFileDescriptor(fd);
	if(f == NULL){
		return -1;
	}	
	else{
		struct file *teller = f->filePointer;
		lock_acquire(&fs_lock);
		position = file_tell(teller);
		lock_release(&fs_lock);
		return position;
		
	}


}

//should be ready to test
void close (int fd) {
	//set file to null (or maybe dont?), cause shouldnt it return null if nothing?
	//get file from list traverser
	//if null, return
	//get lock, file close, release lock
	//remove from list
	//palloc_free_page
	struct fileDescriptor *f = getFileDescriptor(fd);
	if(f == NULL){
		return;	
	}
	struct file *closer = f->filePointer;
	lock_acquire(&fs_lock);
	file_close(closer);
	lock_release(&fs_lock);
	list_remove(&f->element);
	palloc_free_page(f);
}

struct fileDescriptor * getFileDescriptor(int fd){
	//make thread of current thread
	//make a list element to help traverse
	//for(e = head; e != tail; e = next){
	//struct temp = list_entry(e, fileDescriptor, element)
	//if(temp->fdIndex == fd)
		//return temp-> file
	//}
	//return NULL
	struct thread *cur = thread_current();
	struct list_elem *iterator;
	for(iterator = list_begin(&cur->fileDescriptorTable); iterator != list_end(&cur->fileDescriptorTable); iterator = list_next(iterator)){
		struct fileDescriptor *temp = list_entry(iterator, struct fileDescriptor, element);
		if(temp->fdIndex == fd){
			return temp;
		}	
	}

	return NULL;
}
//should be good to test
void halt() {
	shutdown_power_off();
}
void exit(int status) {//implemented like Yerraballi does it
	//double check the exitStatus field name in thread.h
	struct thread *cur = thread_current();
	printf("%s: exit(%d)\n", cur->name, status);
	cur->exitStatus = status;
	thread_exit();
}
tid_t exec(const char *cmd_line) {
	//child_TID == TID_ERROR
	
	lock_acquire(&fs_lock);
	tid_t pid = process_execute(cmd_line);
	lock_release(&fs_lock);
	return pid;
}

int wait(tid_t pid) {
	return process_wait(pid);
}

bool create(const char* file, unsigned initial_size) {
	if(file == NULL){
		exit(-1);
	}
	lock_acquire(&fs_lock);
	bool isCreated = filesys_create(file, initial_size);
	lock_release(&fs_lock);
	return isCreated;
}

bool remove(const char *file) {
	lock_acquire(&fs_lock);
	bool isRemoved = filesys_remove(file);
	lock_release(&fs_lock);
	return isRemoved;
}

int open(const char *file) {
	//allocate memory
	//check if valid, if yes, continue, if not call exit with a negative 1 and then return -1
	//create new file pointer, assign fdIndex == thread current's next value
	//increment the current number of file descriptors
	//populate the struct with these values
	//add struct to list of structs on the thread with list_push_back
	if(file == NULL){
		return -1;
	}
	struct thread *cur = thread_current();
	struct fileDescriptor *f = palloc_get_page(0);
	f->fdIndex = cur->fdLength; //double check field name, and ask TA can I start at 0 with this, because if we need to read or write with standard, we don't LOOK for 0,1
	cur->fdLength = cur->fdLength + 1;//initialization issue
	//thread_current()->fdLength++;
	lock_acquire(&fs_lock);
	struct file *temp = filesys_open(file);
	f->filePointer = temp;
	lock_release(&fs_lock);
	if(temp == NULL){
		return -1;
	}
	list_push_back(&cur->fileDescriptorTable, &f->element);
	return f->fdIndex;

}

int filesize(int fd) {
	//populate file* by traversiing the list looking for fd
	//if null, return 0? -1?
	//else acquire lock, get file_length, release lock
	//return value

	struct fileDescriptor *f = getFileDescriptor(fd);
	if(f == NULL){
		return -1;
	}
	else{
		struct file *temp = f->filePointer;
		int returnSize;
		lock_acquire(&fs_lock);
		returnSize = file_length(temp);
		lock_release(&fs_lock);
		return returnSize;
	}


}

int read(int fd, void *buffer, unsigned size) {
	//check valid memory address
	//if 0, input_getc() repeatedly for buffer length (while/for loop)
	//else, get the file from listTraverse
	//if null, return -1
	//else get lock, file_read, release lock, return result of file_read
	
	int returnSize;

	if(fd == 0){
		int returnSize = 0;
		char *bufferCopy = (char *) buffer;
		for(int a = 0; a < (int) size; a++){
			*(bufferCopy + a) = input_getc();
		}
		return returnSize;
	}
	else{
		struct fileDescriptor *f = getFileDescriptor(fd);
		
		if(f == NULL){
			return -1;		
		}
		else{
			lock_acquire(&fs_lock);
			returnSize = file_read(f->filePointer, buffer, size);
			lock_release(&fs_lock);	
			return returnSize;
		}
	}

	
}


/*

void check_ptr(const void *ptr) {

	if(!is_user_vaddr(ptr)) {
		exit(-1);
	}

	else if(pagedir_get_page(thread_current()->pagedir, ptr) == NULL) {
		exit(-1);
	}
} */

