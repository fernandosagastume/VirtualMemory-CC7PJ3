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

//Install a new page
static bool install_page (void *upage, void *kpage, bool writable);


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
  bool success = pagedir_set_page(curr->pagedir, SPTE->user_vaddr, 
                                  frame, SPTE->is_swap_W);
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

/*Basicamente lo que se hace en esta función es lo que ya se hacia
  en la función de load_segment() pero que dado a que ahora todo se 
  hace con lazy loading, ahora solo se carga lo que se este requiriendo
  en un momento dado en el run time.*/
bool load_from_file_SPTE(struct sup_page_table_entry* SPTE){

  //Setea la posición del archivo en el offset dado.
  file_seek(SPTE->exe, SPTE->file_offset);
  //Se obtiene una nueva página y se guarda en la frame table
  void* buffer = get_pageFT(PAL_USER);

  if(!buffer)
    return false;

  uint32_t bytes_read = file_read(SPTE->exe, buffer, SPTE->file_read_bytes);
    /*Si el tamaño del exe no es igual al que se tiene guardado en el SPTE, 
      se devuelve falso y se libera la página obtenida al principio*/
    if(bytes_read != SPTE->file_read_bytes){
      free_frameTable(buffer);
      return false;
    }

    memset (buffer + SPTE->file_read_bytes, 0, SPTE->file_zero_bytes);

    /*Añade una nueva pagina al address space del proceso*/
      if (!install_page (SPTE->user_vaddr, buffer, SPTE->file_writable)) 
        {
          free_frameTable(buffer);
          return false; 
        }
  //Si se llega a este punto la página ya esta loaded
  SPTE->page_loaded = true;

  return true;
}

bool load_from_MMAP_file_SPTE(struct sup_page_table_entry* SPTE){
  //Setea la posición del archivo en el offset dado.
  //file_seek(SPTE->exe, SPTE->file_offset);
  //printf(" ofs -> %d\n", SPTE->file_offset);
  //Se obtiene una nueva página y se guarda en la frame table
  void* buffer = get_pageFT(PAL_USER);

  if(!buffer)
    return false;

  uint32_t bytes_read = file_read(SPTE->exe, buffer, SPTE->file_read_bytes);
    /*Si el tamaño del exe no es igual al que se tiene guardado en el SPTE, 
      se devuelve falso y se libera la página obtenida al principio*/
    if(bytes_read != SPTE->file_read_bytes){
      free_frameTable(buffer);
      return false;
    }

    memset (buffer + SPTE->file_read_bytes, 0, PGSIZE - SPTE->file_read_bytes);

    /*Añade una nueva pagina al address space del proceso*/
      if (!install_page (SPTE->user_vaddr, buffer, true)) 
        {
          free_frameTable(buffer);
          return false; 
        }
  //Si se llega a este punto la página ya esta loaded
  SPTE->page_loaded = true;

  return true;
}

bool add_EXE_to_SPTE (struct file *file, off_t ofs, void* upage,
                      uint32_t read_bytes, uint32_t zero_bytes,
                      bool writable){

  struct sup_page_table_entry* SPTE;
  //Se crea una sup page entry para la dirección de memoria upage
  SPTE = malloc(sizeof(struct  sup_page_table_entry));
  //Se agrega la información dada en la SPTE
  if(SPTE){
    SPTE->user_vaddr = upage;
    SPTE->exe = file;
    SPTE->file_offset = ofs;
    SPTE->file_read_bytes = read_bytes;
    SPTE->file_zero_bytes = zero_bytes;
    SPTE->file_writable = writable;
    SPTE->page_loaded = false;
    SPTE->status = FROM_EXE;

  }
  else
    return false;

  struct hash_elem* he;
  //Se inserta el entry en la supplemental page table del thread actual
  he = hash_insert(&thread_current()->SPT, &SPTE->spte_elem);

  if( he)
    return false;

  return true;
}

bool add_MMAP_EXE_to_SPTE (struct file *file, off_t ofs, void* upage,
                      uint32_t read_bytes){
  struct sup_page_table_entry* SPTE;
  //Se crea una sup page entry para la dirección de memoria upage
  SPTE = malloc(sizeof(struct  sup_page_table_entry));
  //Se agrega la información dada en la SPTE
  if(SPTE){
    SPTE->user_vaddr = upage;
    SPTE->exe = file;
    SPTE->file_offset = ofs;
    SPTE->file_read_bytes = read_bytes;
    SPTE->page_loaded = false;
    SPTE->status = MMAP;

  }
  else
    return false;

  struct hash_elem* he;
  //Se inserta el entry en la supplemental page table del thread actual
  he = hash_insert(&thread_current()->SPT, &SPTE->spte_elem);

  if(he)
    return false;

  return true;
}

/* Adds a mapping from user virtual address UPAGE to kernel
   virtual address KPAGE to the page table.
   If WRITABLE is true, the user process may modify the page;
   otherwise, it is read-only.
   UPAGE must not already be mapped.
   KPAGE should probably be a page obtained from the user pool
   with palloc_get_page().
   Returns true on success, false if UPAGE is already mapped or
   if memory allocation fails. */
static bool
install_page (void *upage, void *kpage, bool writable)
{
  struct thread *t = thread_current ();

  /* Verify that there's not already a page at that virtual
     address, then map our page there. */
  return (pagedir_get_page (t->pagedir, upage) == NULL
          && pagedir_set_page (t->pagedir, upage, kpage, writable));
}