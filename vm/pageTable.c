#include "vm/pageTable.h"
#include "threads/vaddr.h"
#include "threads/thread.h"
#include "filesys/file.h"
#include "userprog/process.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "lib/kernel/hash.h"


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

bool load_from_swap_SPTE(struct sup_page_table_entry* SPTE){
  return true;
}
bool load_from_file_SPTE(struct sup_page_table_entry* SPTE){
  return true;
}