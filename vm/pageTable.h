#ifndef PAGE_H
#define PAGE_H

#include "filesys/file.h"
#include "threads/palloc.h"
#include "lib/kernel/hash.h"

/*Las siguientes definiciones son para indicar el estado en el que 
  se encuentra la página en la SPTE*/

#define IN_SWAP 1 //La página se encuentra en un swap slot
#define PAGE_FILE 2 //Esta activa en memoria
#define IN_MEMORY 4 //En un .exe


/* Supplemental page table element. */
struct sup_page_table_entry {

  void* user_vaddr; //User virtual address asociada a la SPTE.
  bool dirty; //Verdadero si se escribió en la página.
  bool accessed; //Verdadero si la página fue accesada para lectura/escritura.
  bool page_loaded; //Verdadero si la página esta cargada en la memoria
  uint8_t status; //Indica el estado de la página.
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

//----------------------------------------------------------------------
/*Funcionaes que se usan para cargar datos de una página en 
su SPTE, usado para el proceso de reclamation.*/

bool load_from_swap_SPTE(struct sup_page_table_entry* SPTE);
bool load_from_file_SPTE(struct sup_page_table_entry* SPTE);
//----------------------------------------------------------------------

#endif /* vm/pageTable.h */