#include <stdbool.h>
#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);
void pointers_validation(void* vaddr);
void syscall_halt (void); 
void syscall_exit (int exit_status);
bool syscall_create(const char* file, unsigned initial_size);
bool syscall_remove(const char* file);
int syscall_open(const char* file);
int syscall_filesize(int fd);
int syscall_read(int fd, void* buffer, unsigned size);
int syscall_write(int fd, const void* buffer, unsigned size);
void syscall_seek(int fd, unsigned position);
unsigned syscall_tell(int fd);
void syscall_close(int fd);
#endif /* userprog/syscall.h */
