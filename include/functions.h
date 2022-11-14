#ifndef FUNCTIONS
#define FUNCTIONS

int size_of_block(int size);
int free_list_index(int size);
void free_list_insert(sf_block *pointer, int index);
void set_block_size_length(sf_block *pointer, size_t num);
size_t get_block_size_length(sf_block *pointer);
void copy_header_to_footer(sf_block *pointer);
int get_allocation(sf_block *pointer);
void set_allocation(sf_block *pointer, size_t allo);
void free_list_remove(sf_block *pointer);
int grow_heap(size_t bytesNeeded);
void coalesece(sf_block * block1, sf_block *block2);

#endif

