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
#include "userprog/mmap.h"
#include "vm/pageTable.h"
#include "lib/kernel/hash.h"

static void syscall_handler (struct intr_frame *);
static bool valid_upointer(const void *vaddr);
void* stackp;
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

void pointers_validation(const void* vaddr){
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
	stackp = f->esp;
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
		case SYS_MMAP:
		{
			int* esp = (int *)f->esp;
			//Se obtienen los argumentos del stack
			for (int i = 0; i < 2; i++) {
			    int *ag = ((esp + i) + 1);
			    //Se valida que se este en user space
			    pointers_validation(ag);
			    argSt[i] = *ag;
			}

			f->eax = syscall_mmap((int)argSt[0], (void *)argSt[1]);

			break;
		}
		case SYS_MUNMAP:
		{

			int* esp = (int *)f->esp;
			int *ag = ((esp) + 1);
			//Se valida que se este en user space
			pointers_validation(ag);
			argSt[0] = *ag;

			syscall_munmap((mapid_t)argSt[0]); 
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

		if(!valid_upointer(cmd_line))
			syscall_exit(-1);

	lock_acquire(&lockFS);
	pid_t cpid = process_execute(cmd_line);
	lock_release(&lockFS);

	return cpid;
}

int syscall_wait(pid_t pid)
{
	//printf("ESTE ES EL PID -> %d\n", pid);
	return process_wait((tid_t)pid);
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

//	if(!valid_upointer(file)){
//		syscall_exit(-1);
//	}

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

//------------------------------DEFINICIONES MMAP.H---------------------------------------//

unsigned mmap_file_hash (const struct hash_elem * e, void * aux UNUSED){
	struct mmap_info* mmapI;
	mmapI = hash_entry(e, struct mmap_info, mmap_elem);
	unsigned hash_value = hash_bytes(&mmapI->mapid, sizeof mmapI->mapid);
	return hash_value;
}

bool 
mmap_file_less (const struct hash_elem* a, 
				const struct hash_elem* b, 
				void* aux UNUSED){

	const struct mmap_info* mmapA;
	mmapA = hash_entry(a, struct mmap_info, mmap_elem);
	const struct mmap_info* mmapB;
	mmapB = hash_entry(b, struct mmap_info, mmap_elem);
	return mmapA->mapid < mmapB->mapid;
}

mapid_t 
insert_mmap_file (void* addr, struct file* exe, int32_t length){
	struct mmap_info* mmapex;
	mmapex = malloc(sizeof(struct mmap_info));
	//Return error si no se pudo allocate memory
	if(!mmapex)
		return -1;

	struct thread* curr = thread_current();
	mmapex->mapid = curr->mmap_counter++;
	mmapex->exe = exe;
	mmapex->base_addr = addr;

	int page_ofs, page_count;
	/*Se itera para saber la cantidad de páginas son requeridas para el
	 archivo y se inserta una SPTE que contiene cada página*/
	for (page_count = 0; length > 0; ++page_count)
	{	
		size_t read_bytes;
		/*Se verifica que no se pase del valor arbitrario definido para 
		cada página*/
		if(length < PGSIZE)
			read_bytes = length;
		else
			read_bytes = PGSIZE;
		//Se añade la información de cada página a una SPTE
		bool success = add_MMAP_EXE_to_SPTE(exe, page_ofs, addr, read_bytes);
		if(!success)
			return -1;

		page_ofs = page_ofs + PGSIZE;
		length = length - PGSIZE;
		addr = addr + PGSIZE;
	}

	//Se añade cuantas páginas se requiere para el archivo
	mmapex->page_count = page_count;
	//Se guarda todo en la hash table contenido en el proceso actual
	struct hash_elem* he;
	he = hash_insert(&curr->mmap_files, &mmapex->mmap_elem);

	if(he!=NULL)
		return -1;

	mapid_t mapid_ex = mmapex->mapid;

	return mapid_ex;

}

void 
mmap_files_totally_destroy(struct mmap_info* mmap_info_){

  	int page_count = mmap_info_->page_count;
  	int page_ofs = 0;
  	struct sup_page_table_entry SupPTE;
  	struct hash_elem *hashe;
  	struct thread* curr = thread_current();
  	struct sup_page_table_entry* SPTE;
  	while (page_count > 0)
    {
       page_count--;

      SupPTE.user_vaddr = mmap_info_->base_addr + page_ofs;
      hashe = hash_delete (&curr->SPT, &SupPTE.spte_elem);
      if (hashe) {
	  SPTE = hash_entry(hashe, struct sup_page_table_entry, spte_elem);
	  bool loaded = SPTE->page_loaded;
	   /*Se verifica si la página esta dirty, si lo está entonces 
      	se escribe todo de regreso a disco, de lo contrario no pasa
      	nada.*/
	  if (loaded && pagedir_is_dirty (curr->pagedir, SPTE->user_vaddr) && (SPTE->file_offset >= 0))
	    {
	      //Se escribe todo de regreso a disco
	      lock_acquire (&lockFS);
	      file_seek (SPTE->exe, SPTE->file_offset);
	      file_write (SPTE->exe, SPTE->user_vaddr, SPTE->file_read_bytes);
	      lock_release (&lockFS);
	    }
	}
     page_ofs = page_ofs + PGSIZE;
    }

  	lock_acquire (&lockFS);
  	file_close (mmap_info_->exe);
  	lock_release (&lockFS);

}

void remove_mmap_file(mapid_t mapid){
	struct mmap_info mmapI;
	mmapI.mapid = mapid;
	struct hash_elem* hashe;
	hashe = hash_delete(&thread_current()->mmap_files, &mmapI.mmap_elem);
	if(hashe){
		struct mmap_info* mmapInfo = hash_entry(hashe, struct mmap_info, mmap_elem);
		mmap_files_totally_destroy(mmapInfo);
	}
}

/*Función destructrora cuando se desea destruir un elemento
  de la hash table dada*/
void mmap_destructor_func (struct hash_elem* e, void* aux UNUSED){
	struct mmap_info* mmapi = hash_entry (e, struct mmap_info, mmap_elem);
  	mmap_files_totally_destroy(mmapi);
}

/*Se destruye la hash table con los mmap files*/
void destroy_mmap_files (struct hash* h){
	hash_destroy (h, mmap_destructor_func);
}

//------------------------------DEFINICIONES MMAP.H---------------------------------------//

mapid_t syscall_mmap (int fd, void *addr)
{

  struct thread* curr = thread_current();

  /*A call to mmap may fail if the file open as fd has a length of zero bytes. 
  It must fail if addr is not page-aligned or if the range of pages mapped overlaps 
  any existing set of mapped pages, including the stack or pages mapped at executable load time. 
  It must also fail if addr is 0, because some Pintos code assumes virtual page 0 is not mapped.
  Finally, file descriptors 0 and 1, representing console input and output, are not mappable.*/
  if (addr == NULL || addr == 0 || pg_ofs(addr) != 0){
    return -1;
  }

  if(fd == 0 || fd == 1)
  	return -1;
  	
	struct fileDescriptor* fd_t = NULL;
  	//List elem para iterar en la lista
	struct list_elem* iter_;
	if(!list_empty(&curr->fdList)){
		for (iter_ = list_front(&curr->fdList); iter_ != NULL; iter_ = iter_->next){
			//Se verifica que el file descriptor esta abierto y el dueño es el thread actual
		    struct fileDescriptor* fdstr = list_entry(iter_, struct fileDescriptor, felem);
		    int fdCT = fdstr->fileDescriptor;
		    //Se encontró una ocurrencia
		    if (fdCT == fd) {
		    	fd_t = fdstr;
		        break;
	      	}
	  	}
  	}
  //No se encontró el file descriptor
  if (!fd_t)
    return -1;

  int32_t file_len = file_length (fd_t->file);
  //Se verifica que el length sea mayor a zero bytes
  if (file_len <= 0)
    return -1;

  /*Se verifica si hay suficiente espacio para el archivo*/
  int ofs = 0;
  while (ofs < file_len){
    struct sup_page_table_entry* SPTE  = get_SPTE(addr + ofs, &curr->SPT);
    //Se verifica que no haya alguna user addr in the range 
    if(SPTE)
		return -1;

	void* pd_page = pagedir_get_page(curr->pagedir, addr + ofs);
    /*Se verifica que no haya una dirección fisica mapeada a 
    alguna dirección en el rango*/
    if (pd_page){
		return -1;
    }

    ofs = ofs + PGSIZE;
    }

  /* Add an entry in memory mapped files table, and add entries in
     supplemental page table iteratively which is in mmfiles_insert's
     semantic.
     If success, it will return the mapid;
     otherwise, return -1 */
  lock_acquire (&lockFS);
  struct file* reopened_file = file_reopen(fd_t->file);
  lock_release (&lockFS);
  mapid_t mpid;

  if(!reopened_file)
  	mpid = -1;
  else
  	mpid = insert_mmap_file(addr, reopened_file, file_len);

  return mpid;
}

void syscall_munmap(mapid_t mapid){
	//Remueve el mmap con ese mapid asociado y destruye toda
	//la información disponible en el
	remove_mmap_file(mapid);
	return;
}