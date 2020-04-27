#ifndef SWAP_H
#define SWAP_H

#include <stddef.h>

//Get the block device when we initialize our swap code
void swap_init(void);
void read_from_swap(void* frame, int index);
size_t write_from_swap(void* frame);

#endif /* vm/swap.h */