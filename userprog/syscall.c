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
#include "userprog/process.h"
#include <string.h>

static void syscall_handler (struct intr_frame *);
static bool valid_upointer(const void *vaddr);

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

/*Struct para asociar un map id con una región en la 
  memoria y un archivo*/
struct mmap_info
  {
    mapid_t mapid; /*map id.*/
    struct file* exe;
    void* base_addr; /*La dirección inicio del memory mapping*/
    size_t page_count; /*Cuenta cuantas páginas han sido mapeadas*/
  	struct hash_elem mmap_elem;
  };

//----------------------------------------------------------

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&lockFS);
}

void pointers_validation(const void* vaddr){
	struct thread* curr = thread_current();
	//Se verifica que estemos en user space
	if(!is_user_vaddr(vaddr)){

		syscall_exit(-1);
	}

	void* code_seg = (void *)0x08048000;
	if(!vaddr || vaddr < code_seg){
		syscall_exit(-1);
	}
}

static bool
valid_upointer (const void *vaddr)
{
  struct thread *curr = thread_current();
  if (vaddr && is_user_vaddr(vaddr))
    {
      return (pagedir_get_page (curr->pagedir, vaddr)) != NULL;
    }
  return false;
}

static void
syscall_handler (struct intr_frame *f UNUSED) {

	struct thread* curr = thread_current();
	const void* vaddr = (const void *)f->esp;
	//Se verifica que estemos en user space
	if(!valid_upointer(vaddr) || !valid_upointer(vaddr + 1) || !valid_upointer(vaddr+2) 
		|| !valid_upointer(vaddr+3)){
		syscall_exit(-1);
	}

	//Arreglo para guardar los argumentos del stack.
	int argSt[3]; 

	switch(*(int*)f->esp){
		case SYS_HALT:
		{
			syscall_halt();
			break;
		}
		case SYS_EXIT:
		{
			int* esp = (int *)f->esp;
			int *ag = ((esp) + 1);
			//Se valida que se este en user space
			pointers_validation(ag);
			argSt[0] = *ag;

	        syscall_exit((int)argSt[0]);
	
			break;
		}
		case SYS_EXEC:
		{
			//int* esp = (int *)f->esp;
			//Se obtienen los argumentos del stack
			uint32_t* ag = (uint32_t*)f->esp + 1;
			if(ag == NULL)
				syscall_exit(-1);
			//Se valida que se este en user space
			pointers_validation(ag);


			void* physPage = pagedir_get_page (curr->pagedir,(const void *)*ag);

			if (!physPage)
	          	syscall_exit(-1);

	        argSt[0] = (int)physPage;

			f->eax = syscall_exec((const char *)*ag);
			break;
		}
		case SYS_WAIT:
		{
			int* esp = (int *)f->esp;
			int *ag = ((esp) + 1);
			//Se valida que se este en user space
			pointers_validation(ag);
			argSt[0] = *ag;

	        f->eax = syscall_wait((pid_t)argSt[0]);
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
			    const void* arg = (const void *)ag;
			    pointers_validation(arg);
			    argSt[i] = *ag;
			}

			char* buff  = (char *)argSt[1];
			for (int i = 0; i < argSt[2]; i++){
			      pointers_validation((const void *)buff);
			      buff++;
			    }

			void* physPage = pagedir_get_page (curr->pagedir,(const void *)argSt[1]);
			//ASSERT(!physPage);
			if (!physPage)
	          	syscall_exit(-1);
	        
	        argSt[1] = (int)physPage;
      		f->eax = syscall_write(argSt[0], (const void*)argSt[1], (unsigned)argSt[2]);
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

			char* buff  = (char *)argSt[0];
			for (int i = 0; i < argSt[1]; i++){
			      pointers_validation((const void *)buff);
			      buff++;
			    }

			void* physPage = pagedir_get_page (curr->pagedir,(const void *)argSt[0]);

			if (!physPage)
	          	syscall_exit(-1);

	        argSt[0] = (int)physPage;

			f->eax = syscall_create((const char*)argSt[0], (unsigned)argSt[1]);
			break;
		}
		case SYS_REMOVE:
		{
			int* esp = (int *)f->esp;
			int *ag = ((esp) + 1);
			//Se valida que se este en user space
			pointers_validation(ag);
			argSt[0] = *ag;
			void* physPage = pagedir_get_page (curr->pagedir,(const void *)argSt[0]);

			if (!physPage)
	          	syscall_exit(-1);

	        argSt[0] = (int)physPage;

	        f->eax = syscall_remove((const char*)argSt[0]);
			break;
		}
		case SYS_OPEN:
		{
			int* esp = (int *)f->esp;
			int *ag = ((esp) + 1);
			//Se valida que se este en user space
			pointers_validation(ag);
			argSt[0] = *ag;
			void* physPage = pagedir_get_page (curr->pagedir,(const void *)argSt[0]);

			if (!physPage)
	          	syscall_exit(-1);

	        argSt[0] = (int)physPage;

	        f->eax = syscall_open((const char*)argSt[0]);
			break;
		}
		case SYS_FILESIZE:
		{
			int* esp = (int *)f->esp;
			int *ag = ((esp) + 1);
			//Se valida que se este en user space
			pointers_validation(ag);
			argSt[0] = *ag;

	        f->eax = syscall_filesize((int)argSt[0]);
			break;
		}
		case SYS_CLOSE:
		{
			int* esp = (int *)f->esp;
			int *ag = ((esp) + 1);
			//Se valida que se este en user space
			pointers_validation(ag);
			argSt[0] = *ag;

	        syscall_close((int)argSt[0]);
			break;
		}
		case SYS_TELL:
		{	
			int* esp = (int *)f->esp;
			int *ag = ((esp) + 1);
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
 
/* Revisa si se intenta acceder a espacios validos */ 
inline bool
valid_vaddr_range(const void *vaddr, unsigned size)
{
	if(vaddr == NULL)
		return false;

	if(!is_user_vaddr(vaddr) || !is_user_vaddr(vaddr + size))
		return false;

	return true;
}

 void 
 syscall_halt (void){
 	shutdown_power_off();
 } 

void 
syscall_exit (int status){
	struct thread * curr_thread = thread_current();
	//printf("ESTE ES EL STATUS -> %d\n", status);
	curr_thread->exit_value = status;
	thread_exit();
}

pid_t syscall_exec(const char *cmd_line)
{

	if(!cmd_line){
		//printf("ESTOY AQUI\n");
		return -1;
	}

	lock_acquire(&lockFS);
	pid_t cpid = process_execute(cmd_line);
	lock_release(&lockFS);

	return cpid;
}

int syscall_wait(pid_t pid)
{
	//printf("ESTE ES EL PID -> %d\n", pid);
	return process_wait(pid);
}


bool 
syscall_create(const char* file, unsigned initial_size){
	//Si el nombre del archivo es incorrecto, se hace exit
	//if(file == NULL){
	//	syscall_exit(-1);//exit_status not successful
	//}
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
	if(!file_){
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
	curr->fdSZ++;
	fd->fileDescriptor = fileDescriptor;
	//Se ingresa el File D. a la lista de file descriptors del thread actual
  	list_push_front(&curr->fdList, &fd->felem);
  	lock_release(&lockFS);
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
	        return fs;
      	   }
  		}
  		lock_release(&lockFS);
	}else{//De forma contraria se devuelve -1 indicando que no hay file descriptors
		lock_release(&lockFS);
		return -1;
	}

	return -1;
}

int 
syscall_read(int fd, void* buffer, unsigned size){
	//-1 en caso de que no se pueda leer el file
	int bytes_to_read = 0;
	lock_acquire(&lockFS);

	//fd = 0 lee del teclado usand input_getc
	if(fd == 0){
		//Se castea porque input getc devuelve uint8_t
		bytes_to_read = (int)input_getc();
		lock_release(&lockFS);
		return bytes_to_read;
	}

	struct thread* curr = thread_current();
	//fd = 1(STDOUT_FILENO), no hay nada para leer
	//porque se le paso un fd de escritura o
	//no hay fd en el thread actual
	if(list_empty(&curr->fdList) || fd == 1){
		bytes_to_read = 0;
		lock_release(&lockFS);
		return bytes_to_read;
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
	        return bytes_to_read;
      	   }
  		}
  	lock_release(&lockFS);
  	bytes_to_read = -1; 
  	return bytes_to_read;
}

int 
syscall_write(int fd, const void* buffer, unsigned size){
	int bytes_to_write = 0; 
	lock_acquire(&lockFS);
	struct thread* curr = thread_current();

	//fd = 1 escribe a consola con la funcion putbuf 
	//(funcion de kernel/console.c)
	if(fd == 1){	
		putbuf(buffer,size);
		lock_release(&lockFS);
		return size;
	}

	//fd = 0(STDIN_FILENO), no hay nada para escribir 
	//porque se le paso un fd de escritura o
	//no hay fd en el thread actual
	if(list_empty(&curr->fdList) || fd == 0){
		bytes_to_write = 0;
		lock_release(&lockFS); 
		return bytes_to_write;
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
	        return bytes_to_write;
      	   }
  		}

  	lock_release(&lockFS);
  	return 0;
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
	}else{//De forma contraria hace return indicando que no hay file descriptors
		lock_release(&lockFS);
		return;
	}
	lock_release(&lockFS);
	return;
}

unsigned 
syscall_tell(int fd){
	unsigned pos = 0;
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
	        return pos;
      	   }
  		}
	}else{//De forma contraria se libera el lock
		lock_release(&lockFS);
		pos = -1;
		return pos;
	}

	lock_release(&lockFS);
	pos = -1;
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
	}else{//De forma contraria hace return indicando que no hay file descriptors
		lock_release(&lockFS);
		return;
	}
	lock_release(&lockFS);
  		return;
}