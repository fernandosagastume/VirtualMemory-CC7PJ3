#include "userprog/syscall.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "threads/malloc.h"
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "threads/synch.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "devices/input.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"

static void syscall_handler (struct intr_frame *);

//---------------------------------------------------------

//Se crea una variable tipo lock para el fylesystem
struct lock lockFS;

/*Struct que lleva control de cada file, file descriptor
y la lista de file descriptors del thread actual*/
 struct fileDescriptor
{
	int fileDescriptor;
	struct file* file;
    struct list_elem felem;
};

//----------------------------------------------------------

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&lockFS);
}

void pointers_validation(void* vaddr){
	struct thread* curr = thread_current();
	//Se verifica que estemos en user space
	if(!is_user_vaddr(vaddr))
		syscall_exit(-1);

	else if(!(pagedir_get_page (curr->pagedir, vaddr)))
		syscall_exit(-1);;
}

static void
syscall_handler (struct intr_frame *f UNUSED) {

	struct thread* curr = thread_current();
	const void* vaddr = (const void *)f->esp;
	//Se verifica que estemos en user space
	if(!is_user_vaddr(vaddr))
		syscall_exit(-1);

	else if(!(pagedir_get_page (curr->pagedir, vaddr)))
		syscall_exit(-1);

	//Arreglo para guardar los argumentos del stack.
	int *argSt = (int *)malloc(sizeof(int)*3); 

	switch(*(int*)f->esp){
		case SYS_HALT:
		{
			syscall_halt();
			break;
		}
		case SYS_EXIT:
		{
			break;
		}
		case SYS_EXEC:
		{
			break;
		}
		case SYS_WAIT:
		{
			break;
		}
//------------------------------------------
		case SYS_WRITE:
		{
			int* esp = (int *)f->esp;
			//Se obtienen los argumentos del stack
			for (int i = 0; i < 3; i++) {
			    int *ag = ((esp + i) + 1);
			    //Se valida que se este en user space
			    pointers_validation(ag);
			    argSt[i] = *ag;
			}

			void* physPage = pagedir_get_page (curr->pagedir,(const void *)argSt[1]);
			argSt[1] = (int)physPage;
			if (physPage == NULL)
	        {
	          	syscall_exit(-1);
	        }
      		f->eax = syscall_write((int)argSt[0], (void*)argSt[1], (unsigned)argSt[2]);
			break;
		}
		case SYS_READ:
		{
			int* esp = (int *)f->esp;
			//Se obtienen los argumentos del stack
			for (int i = 0; i < 3; i++) {
			    int *ag = ((esp + i) + 1);
			    //Se valida que se este en user space
			    pointers_validation(ag);
			    argSt[i] = *ag;
			}

			void* physPage = pagedir_get_page (curr->pagedir,(const void *)argSt[0]);
			argSt[0] = (int)physPage;

			if (physPage == NULL)
	        {
	          	syscall_exit(-1);
	        }
	        
      		f->eax = syscall_read((int)argSt[0], (void *)argSt[1], (unsigned)argSt[2]);
			break;
		}
		case SYS_CREATE:
		{
			int* esp = (int *)f->esp;
			//Se obtienen los argumentos del stack
			for (int i = 0; i < 2; i++) {
			    int *ag = ((esp + i) + 1);
			    //Se valida que se este en user space
			    pointers_validation(ag);
			    argSt[i] = *ag;
			}

			void* physPage = pagedir_get_page (curr->pagedir,(const void *)argSt[0]);
			argSt[0] = (int)physPage;

			if (physPage == NULL)
	        {
	          	syscall_exit(-1);
	        }

			f->eax = syscall_create((const char*)argSt[0], (unsigned)argSt[1]);
			break;
		}
		case SYS_REMOVE:
		{
			int* esp = (int *)f->esp;
			int *ag = ((esp + 1) + 1);
			//Se valida que se este en user space
			pointers_validation(ag);
			argSt[0] = *ag;
			void* physPage = pagedir_get_page (curr->pagedir,(const void *)argSt[0]);
			argSt[0] = (int)physPage;

			if (physPage == NULL)
	        {
	          	syscall_exit(-1);
	        }

	        f->eax = syscall_remove((const char*)argSt[0]);
			break;
		}
		case SYS_OPEN:
		{
			int* esp = (int *)f->esp;
			int *ag = ((esp + 1) + 1);
			//Se valida que se este en user space
			pointers_validation(ag);
			argSt[0] = *ag;
			void* physPage = pagedir_get_page (curr->pagedir,(const void *)argSt[0]);
			argSt[0] = (int)physPage;

			if (physPage == NULL)
	        {
	          	syscall_exit(-1);
	        }

	        f->eax = syscall_open((const char*)argSt[0]);
			break;
		}
		case SYS_FILESIZE:
		{
			int* esp = (int *)f->esp;
			int *ag = ((esp + 1) + 1);
			//Se valida que se este en user space
			pointers_validation(ag);
			argSt[0] = *ag;

	        f->eax = syscall_filesize((int)argSt[0]);
			break;
		}
		case SYS_CLOSE:
		{
			int* esp = (int *)f->esp;
			int *ag = ((esp + 1) + 1);
			//Se valida que se este en user space
			pointers_validation(ag);
			argSt[0] = *ag;

	        syscall_close((int)argSt[0]);
			break;
		}
		case SYS_TELL:
		{	
			int* esp = (int *)f->esp;
			int *ag = ((esp + 1) + 1);
			//Se valida que se este en user space
			pointers_validation(ag);
			argSt[0] = *ag;

	        f->eax = syscall_tell((int)argSt[0]);
			break;
		}
		case SYS_SEEK:
		{
			int* esp = (int *)f->esp;
			//Se obtienen los argumentos del stack
			for (int i = 0; i < 2; i++) {
			    int *ag = ((esp + i) + 1);
			    //Se valida que se este en user space
			    pointers_validation(ag);
			    argSt[i] = *ag;
			}

			syscall_seek((int)argSt[0], (unsigned)argSt[1]);
			break;
		}
		default:
		{
			syscall_exit(-1);
			break;
		}
	}
}
 void 
 syscall_halt (void){
 	shutdown_power_off();
 } 

void 
syscall_exit (int exit_status UNUSED){
	thread_exit();
}


bool 
syscall_create(const char* file, unsigned initial_size){
	//Si el nombre del archivo es incorrecto, se hace exit
	if(file == NULL){
		syscall_exit(-1);//exit_status not successful
	}
	lock_acquire(&lockFS);
	bool success = filesys_create(file, initial_size);
	lock_release(&lockFS);
	return success;
}

bool 
syscall_remove(const char* file){
	lock_acquire(&lockFS);
	bool success = filesys_remove(file);
	lock_release(&lockFS);
	return success;
}

int 
syscall_open(const char* file){
	lock_acquire(&lockFS);
	int fileDescriptor;
	struct file* file_ = filesys_open(file);
	if(file_ == NULL){
		//The file could not be opened, file descriptor -> -1
		fileDescriptor = -1;
		lock_release(&lockFS);
		return fileDescriptor;
	}

	int fdSize = sizeof(struct fileDescriptor);
	struct fileDescriptor* fd = malloc(fdSize);
	struct thread* curr = thread_current();
	//Se guarda el archivo que se intenta abrir
	fd->file = file_;
	//El fileDescriptor del thread actual
	fileDescriptor = curr->fdSZ;
	//Se incrementa el file descriptor size en caso de el thread abra mas archivos
	curr->fdSZ+=1;
	fd->fileDescriptor = fileDescriptor;
	//Se ingresa el File D. a la lista de file descriptors del thread actual
  	list_push_front(&curr->fdList, &fd->felem);
  	lock_release(&lockFS);
  	free(fd);
	return fileDescriptor;
}

int 
syscall_filesize(int fd){
	//File size
	int fs;
	lock_acquire(&lockFS);
	struct thread* curr = thread_current();
	//Se verifica si hay file descriptors asociados al thread actual
	if(!list_empty(&curr->fdList)){
		//List elem para iterar en la lista
		struct list_elem* iter_;
		for (iter_ = list_front(&curr->fdList); iter_ != NULL; iter_ = iter_->next){
		 //Se verifica que el file descriptor esta abierto y el dueño es el thread actual
	      struct fileDescriptor* fd_t = list_entry(iter_, struct fileDescriptor, felem);
	      int fdCT = fd_t->fileDescriptor;

	      if (fdCT == fd) {
	        struct file* fileFD = fd_t->file;
	        fs = (int)file_length(fileFD);
	        lock_release(&lockFS);
	        break;
      	   }
  		}
  		lock_release(&lockFS);
	}else{//De forma contraria se devuelve -1 indicando que no hay file descriptors
		lock_release(&lockFS);
		fs = -1;
	}

	return fs;
}

int 
syscall_read(int fd, void* buffer, unsigned size){
	//-1 en caso de que no se pueda leer el file
	int bytes_to_read = -1;
	lock_acquire(&lockFS);

	struct thread* curr = thread_current();
	//fd = 1(STDOUT_FILENO), no hay nada para leer
	//porque se le paso un fd de escritura o
	//no hay fd en el thread actual
	if(list_empty(&curr->fdList) || fd == 1){
		bytes_to_read = 0;
		lock_release(&lockFS);
	}

	//fd = 0 lee del teclado usand input_getc
	else if(fd == 0){
		//Se castea porque input getc devuelve uint8_t
		bytes_to_read = (int)input_getc();
		lock_release(&lockFS);
	}

		//List elem para iterar en la lista
		struct list_elem* iter_;
		for (iter_ = list_front(&curr->fdList); iter_ != NULL; iter_ = iter_->next){
		 //Se verifica que el file descriptor esta abierto y el dueño es el thread actual
	      struct fileDescriptor* fd_t = list_entry(iter_, struct fileDescriptor, felem);
	      int fdCT = fd_t->fileDescriptor;

	      if (fdCT == fd) {
	        struct file* fileFD = fd_t->file;
	        //Se lee el archivo
	        bytes_to_read = (int)file_read(fileFD,buffer,size);
	        lock_release(&lockFS);
	        break;
      	   }
  		}
  	lock_release(&lockFS);
  	return bytes_to_read;
}

int 
syscall_write(int fd, const void* buffer, unsigned size){
	int bytes_to_write = 0; 
	lock_acquire(&lockFS);

	struct thread* curr = thread_current();

	//fd = 0(STDIN_FILENO), no hay nada para escribir 
	//porque se le paso un fd de escritura o
	//no hay fd en el thread actual
	if(list_empty(&curr->fdList) || fd == 0){
		bytes_to_write = 0;
		lock_release(&lockFS);
	}
	//fd = 1 escribe a consola con la funcion putbuf 
	//(funcion de kernel/console.c)
	else if(fd == 1){
		putbuf(buffer,(size_t)size);
		bytes_to_write = (int)size;
		lock_release(&lockFS);
	}

		//List elem para iterar en la lista
		struct list_elem* iter_;
		for (iter_ = list_front(&curr->fdList); iter_ != NULL; iter_ = iter_->next){
		 //Se verifica que el file descriptor esta abierto y el dueño es el thread actual
	      struct fileDescriptor* fd_t = list_entry(iter_, struct fileDescriptor, felem);
	      int fdCT = fd_t->fileDescriptor;

	      if (fdCT == fd) {
	        struct file* fileFD = fd_t->file;
	        //Se escribe el archivo
	        bytes_to_write = (int)file_write(fileFD,buffer,size);
	        lock_release(&lockFS);
	        break;
      	   }
  		}

  	lock_release(&lockFS);
  	return bytes_to_write;
}

void 
syscall_seek(int fd, unsigned position){
	lock_acquire(&lockFS);

	struct thread* curr = thread_current();

	if(!list_empty(&curr->fdList)){
		//List elem para iterar en la lista
		struct list_elem* iter_;
		for (iter_ = list_front(&curr->fdList); iter_ != NULL; iter_ = iter_->next){
		 //Se verifica que el file descriptor esta abierto y el dueño es el thread actual
	      struct fileDescriptor* fd_t = list_entry(iter_, struct fileDescriptor, felem);
	      int fdCT = fd_t->fileDescriptor;

	      if (fdCT == fd) {
	        struct file* fileFD = fd_t->file;
	        //Se busca en el archivo y en la position indicada
	        file_seek(fileFD, position);
	        lock_release(&lockFS);
	        return;
      	   }
  		}
  		lock_release(&lockFS);
  		return;
	}else{//De forma contraria hace return indicando que no hay file descriptors
		lock_release(&lockFS);
		return;
	}

}

unsigned 
syscall_tell(int fd){
	unsigned pos = -1;
	lock_acquire(&lockFS);

	struct thread* curr = thread_current();

	if(!list_empty(&curr->fdList)){
		//List elem para iterar en la lista
		struct list_elem* iter_;
		for (iter_ = list_front(&curr->fdList); iter_ != NULL; iter_ = iter_->next){
		 //Se verifica que el file descriptor esta abierto y el dueño es el thread actual
	      struct fileDescriptor* fd_t = list_entry(iter_, struct fileDescriptor, felem);
	      int fdCT = fd_t->fileDescriptor;

	      if (fdCT == fd) {
	        struct file* fileFD = fd_t->file;
	        //Se busca y se devuelve la siguiente posición a leer o escribir
	        pos = (unsigned)file_tell(fileFD);
	        lock_release(&lockFS);
	        break;
      	   }
  		}
  		lock_release(&lockFS);
	}else{//De forma contraria se libera el lock
		lock_release(&lockFS);
	}

	lock_release(&lockFS);
	return pos;
}

void 
syscall_close(int fd){
	lock_acquire(&lockFS);

	struct thread* curr = thread_current();

	if(!list_empty(&curr->fdList)){
		//List elem para iterar en la lista
		struct list_elem* iter_;
		for (iter_ = list_front(&curr->fdList); iter_ != NULL; iter_ = iter_->next){
		 //Se verifica que el file descriptor esta abierto y el dueño es el thread actual
	      struct fileDescriptor* fd_t = list_entry(iter_, struct fileDescriptor, felem);
	      int fdCT = fd_t->fileDescriptor;

	      if (fdCT == fd) {
	        struct file* fileFD = fd_t->file;
	        //Se cierra el archivo
	        file_close(fileFD);
	        //Se remueve el archivo de la lista de file descriptors del thread actual
	        list_remove(&fd_t->felem);

	        lock_release(&lockFS);
	        return;
      	   }
  		}
  		lock_release(&lockFS);
  		return;
	}else{//De forma contraria hace return indicando que no hay file descriptors
		lock_release(&lockFS);
		return;
	}
}