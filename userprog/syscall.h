#include <stdbool.h>
#include <stdint.h>
#include "lib/user/syscall.h"
#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);
void pointers_validation(const void* vaddr);
bool valid_vaddr_range(const void *vaddr, unsigned size);
void syscall_halt (void); 
void syscall_exit (int status);
pid_t syscall_exec(const char *cmd_line);
int syscall_wait(pid_t pid);
bool syscall_create(const char* file, unsigned initial_size);
bool syscall_remove(const char* file);
int syscall_open(const char* file);
int syscall_filesize(int fd);
int syscall_read(int fd, void* buffer, unsigned size);
int syscall_write(int fd, const void* buffer, unsigned size);
void syscall_seek(int fd, unsigned position);
unsigned syscall_tell(int fd);
void syscall_close(int fd);

/*Funciones parte 3*/
mapid_t syscall_mmap(int fd, void* addr);
void syscall_munmap(mapid_t mapid);

#endif /* userprog/syscall.h */