#include "vm/swap.h"
#include "devices/block.h"
#include "lib/kernel/bitmap.h"
#include "threads/vaddr.h"
#include <stdbool.h>
#include <stddef.h>

//Make the swap block global
struct block* global_swap_block;
/* Bitmap del swap slot*/
struct bitmap* swap_bitmap;
//Sectores necesitados por página
#define SECTORxPAGE (PGSIZE/BLOCK_SECTOR_SIZE)

void swap_init(void){
	//Inicialización de swap block
	global_swap_block = block_get_role(BLOCK_SWAP);
	if(!global_swap_block)
		return;

	/*Initializes the bitmap with the appropriate number of bits.
	 Se divide por el número de sectores por página para saber 
	 cuantas páginas puede almacenar un swap block*/
	swap_bitmap = bitmap_create(block_size(global_swap_block)/SECTORxPAGE);
	if(!swap_bitmap)
		return;

	bool value = false;
	//Se incializan todos los bits como falsos
	bitmap_set_all(swap_bitmap, value);
}

/*Now remember that each block device is 512 bytes (defined as BLOCK_SECTOR_SIZE in block.h,and 
 each page is 4096 bytes (defined as PGSIZE in vaddr.h, meaning it will take 8 blocks to hold
 the information of one page. This means if we want to read or write a page into disk using a 
 block, you’ll need to read or write 8 consecutive blocks (PGSIZE / BLOCK_SECTOR_SIZE)*/

void read_from_swap(void* frame, int index){
	//Se castea el frame para uso posterior
	uint8_t* frame_ = (uint8_t *)frame;
	/*Antes de empezar se verifica si no se está tratando de 
	 hacer swap con un bloque libre*/
	bool bloque = bitmap_test(swap_bitmap, index);
	if(bloque == false)
		PANIC("Se está intentando intercambiar con un bloque libre");

	for (int i = 0; i < SECTORxPAGE; ++i){

		block_read(global_swap_block, i + index*SECTORxPAGE,
		 frame_ + (i*BLOCK_SECTOR_SIZE));
	}
	//Se libera del bitmap el swap slot del index dado
	bitmap_flip(swap_bitmap, index);
}

size_t write_from_swap(void* frame){
	//Se castea el frame para uso posterior
	uint8_t* frame_ = (uint8_t *)frame;

	/*Se busca un swap slot que no se este usando y se pone como true
  	indicando que será utilizado*/
	bool value = false;
  	size_t index = bitmap_scan_and_flip (swap_bitmap, 0, 1, value);
  
  	bool swap_full = (index == BITMAP_ERROR);
  	if(swap_full)
  	PANIC("El swap está lleno");
  
  	for (int i = 0; i < SECTORxPAGE; ++i){

		block_read(global_swap_block, i + index*SECTORxPAGE,
		 frame_ + (i*BLOCK_SECTOR_SIZE));
	}
	return index;
}