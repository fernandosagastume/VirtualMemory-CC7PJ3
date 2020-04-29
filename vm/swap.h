#ifndef SWAP_H
#define SWAP_H

#include <stddef.h>

//Get the block device when we initialize our swap code
void swap_init(void);
void read_from_swap(void* frame, size_t index);
size_t write_from_swap(void* frame);
void swap_flip(size_t index);
#endif /* vm/swap.h */