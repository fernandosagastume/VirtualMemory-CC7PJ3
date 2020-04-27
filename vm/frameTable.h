#ifndef VM_FRAME_TABLE_H
#define VM_FRAME_TABLE_H
#include "threads/thread.h"
#include "threads/palloc.h"
#include "vm/pageTable.h"

//a list of frame_table_entry as the page table
struct list frame_table;

struct frame_table_entry {
	void * frame;
	struct thread* thread_owner;
	void* user_vaddr;
	uint32_t* pte;
	struct list_elem frame_elem;	
};

//Se incializan elementos necesarios (Definida en vm/frameTable.c)
void init_frameTable(void);
//Se obtiene la pagina y se guarda en la lista frame_table
void *get_pageFT(enum palloc_flags);
//Remueve los frames de la frame_table (Definida en vm/frameTable.c)
void free_frameTable(void *);
/*Función que se utiliza para abrir espacio para una nueva página
  en caso de que haya que hacer Eviction*/
void* evict_page(void);
/*Se elige la victima a la cual se le hará evict de la memoria cache*/
struct frame_table_entry* select_victim(void);
/*Se guarda el contenido del evicted frame*/
bool evicted_save(struct frame_table_entry* victima);
/*Función para guardar la pte y user virtual address mapeado al frame*/
void getPTEandUvaddr(void* frame, void* upage, uint32_t* pte);
#endif /* vm/frameTable.h */