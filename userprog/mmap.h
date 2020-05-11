#ifndef USERPROG_MMAP_H
#define USERPROG_MMAP_H
#include "threads/thread.h"

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

/*Funciones utilizadas y requeridas para incializar el hash table que 
alberga la suplemetal page table de cada thread*/

/* Computes and returns the hash value for hash element E, given	
   auxiliary data AUX. */
unsigned mmap_file_hash (const struct hash_elem* e, void* aux);
/* Compares the value of two hash elements A and B, given
   auxiliary data AUX.  Returns true if A is less than B, or
   false if A is greater than or equal to B. */
bool mmap_file_less (const struct hash_elem* a, 
					 const struct hash_elem* b, 
					 void* aux);
/*Inserta información en la hash table de mmap files de cada proceso como
también en la supplemental page table*/
mapid_t insert_mmap_file (void* addr, struct file* exe, int32_t length);
/*Remueve información en la hash table de mmap files del mapid correspondiente 
de cada proceso como también en la supplemental page table*/
void remove_mmap_file(mapid_t mapid);
/*Función destructrora cuando se desea destruir un elemento
  de la hash table dada*/
void mmap_destructor_func (struct hash_elem* e, void* aux);
/*Se destruye la hash table con los mmap files*/
void destroy_mmap_files (struct hash* h);
/*Destruye absolutamente todo incluyendo supplemental page table*/
void mmap_files_totally_destroy(struct mmap_info* mmap_info_);

#endif /* userprog/mmap.h */