#include "vm/pageTable.h"
#include "vm/frameTable.h"
#include "vm/swap.h"
#include "threads/vaddr.h"
#include "threads/thread.h"
#include "filesys/file.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"
#include "lib/kernel/hash.h"
#include "threads/malloc.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>


unsigned SPTE_hash (const struct hash_elem* e, void *aux UNUSED){
  struct sup_page_table_entry* SPTE;
  SPTE = hash_entry(e, struct sup_page_table_entry, spte_elem);
  unsigned hash_value = hash_bytes (&SPTE->user_vaddr, sizeof &SPTE->user_vaddr);
  return hash_value;
}

bool SPTE_less (const struct hash_elem* a,
                const struct hash_elem* b,
                void* aux UNUSED){
  const struct sup_page_table_entry* spte_a;
  spte_a = hash_entry (a, struct sup_page_table_entry, spte_elem);
  const struct sup_page_table_entry* spte_b;
  spte_b = hash_entry (b, struct sup_page_table_entry, spte_elem);
  return spte_a->user_vaddr < spte_b->user_vaddr;
}


struct sup_page_table_entry* 
get_SPTE(void* user_vaddr, struct hash* hash_table){
  struct sup_page_table_entry SPTE;
  //La key de la hash_table (user_vaddr) se guarda en la spte
  SPTE.user_vaddr = user_vaddr;
  struct hash_elem* h = &SPTE.spte_elem;
  //Se busca en la hash table un spte asociado
  struct hash_elem* element = hash_find(hash_table, h);
  if(element)
    return hash_entry(element, struct sup_page_table_entry, spte_elem);
  else
    return NULL;
}

bool storeInSPT(struct sup_page_table_entry* SPTE, struct hash* TsupPT){
  bool success = false;

  if(!SPTE)
    success = false;

  struct hash_elem* insert_success = hash_insert(TsupPT, &SPTE->spte_elem);

  if(insert_success)
    success = true;
  return success;
}

void destructor_func (struct hash_elem* e, void* aux UNUSED)
{
  struct sup_page_table_entry *SPTE;
  SPTE = hash_entry (e, struct sup_page_table_entry, spte_elem);

  /*Si existe algo guardado en swap asociado con le SPT, se quita
    del bitmap perteneciente al swap.*/
  if (SPTE->status == IN_SWAP)
    swap_flip(SPTE->swap_index);
  //Se libera memoria
  free(SPTE);
}

void destroy_SPT(struct hash* SPT) 
{
  hash_destroy (SPT, destructor_func);
}

bool load_from_swap_SPTE(struct sup_page_table_entry* SPTE){
  //Se asigna una página en memoria 
  void* frame = get_pageFT(PAL_USER);
  struct thread* curr = thread_current();

  if(!frame)
    return false;
  //Se le añade mapping en el page directory
  bool success = pagedir_set_page(curr->pagedir, SPTE->user_vaddr, frame, SPTE->is_swap_W);
  //Si el memory allocation falla, se saca el frame de la frame table y se devuelve falso
  if(!success){
    free_frameTable(frame);
    return false;
  }
  //Se trae la información guardada en swap
  read_from_swap(frame, SPTE->swap_index);

  if(SPTE->status == IN_SWAP)
    hash_delete(&curr->SPT, &SPTE->spte_elem);

  if(SPTE->status == (IN_SWAP | FROM_EXE)){
    SPTE->page_loaded = true;
    SPTE->status = FROM_EXE; 
  }

  return true;
}
