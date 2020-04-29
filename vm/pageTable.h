#ifndef VM_SUP_PAGE_TABLE_H
#define VM_SUP_PAGE_TABLE_H

#include "filesys/file.h"
#include "threads/palloc.h"
#include "lib/kernel/hash.h"
#include "vm/frameTable.h"

/*Las siguientes definiciones son para indicar el estado en el que 
  se encuentra la página en la SPTE*/

#define IN_SWAP 1 //La página se encuentra en un swap slot
#define FROM_EXE 2 ////En un ejecutable
#define IN_MEMORY 4 //Esta activa en memoria

/* Supplemental page table element. */
struct sup_page_table_entry {

  void* user_vaddr; //User virtual address asociada a la SPTE.
  bool dirty; //Verdadero si se escribió en la página.
  bool accessed; //Verdadero si la página fue accesada para lectura/escritura.
  bool page_loaded; //Verdadero si la página esta cargada en la memoria
  uint8_t status; //Indica el estado de la página.
  size_t swap_index; //Util cuando se necesita guardar en swap
  bool is_swap_W; //Verdadero si no es read-only.
  struct hash_elem spte_elem; 	
};


//----------------------------------------------------------------------
/*Funciones utilizadas y requeridas para incializar el hash table que 
alberga la suplemetal page table de cada thread*/

/* Computes and returns the hash value for hash element E, given	
   auxiliary data AUX. */
unsigned SPTE_hash (const struct hash_elem* e, void* aux);

/* Compares the value of two hash elements A and B, given
   auxiliary data AUX.  Returns true if A is less than B, or
   false if A is greater than or equal to B. */
bool SPTE_less (const struct hash_elem* a,
               	const struct hash_elem* b,
	       		void* aux);
//----------------------------------------------------------------------

/*Función que busca dada una hash table y un user virtual address
  la SPTE asociada a la misma*/
struct sup_page_table_entry* 
get_SPTE(void* user_vaddr, struct hash* hash_table);

/*Función destructrora cuando se desea destruir un elemento
  de la hash table dada*/
void destructor_func (struct hash_elem* e, void* aux UNUSED);

/*Se destruye la Supplemental Page Table asociada dada*/
void destroy_SPT(struct hash* SPT);

/*Función que guarda una supplemental page table creada, en la SPT
 asociada al thread que de quien esta siendo creada (i.e el thread
 owner de un frame)*/
bool storeInSPT(struct sup_page_table_entry* SPTE, struct hash* TsupPT);

//----------------------------------------------------------------------
/*Funcionaes que se usan para cargar datos de una página en 
su SPTE, usado para el proceso de reclamation.*/

bool load_from_swap_SPTE(struct sup_page_table_entry* SPTE);
bool load_from_file_SPTE(struct sup_page_table_entry* SPTE);
//----------------------------------------------------------------------

#endif /* vm/pageTable.h */