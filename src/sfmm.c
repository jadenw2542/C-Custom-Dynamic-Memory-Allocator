/**
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "sfmm.h"
#include "errno.h"
#include "functions.h"

int MIN_BLOCK_SIZE = 32;

void *sf_malloc(size_t size) {

    if(size == 0){ //if the request size is 0
        return NULL;
    }

    int bytesNeeded = size_of_block(size);

    int index_to_store = free_list_index(bytesNeeded);

    int is_heap_init = 0;
    if (sf_mem_end() != sf_mem_start()){
        is_heap_init = 1;
    }

    if(is_heap_init == 0){
        sf_mem_grow();
        int list_index = 0;
        while(list_index < NUM_FREE_LISTS){ // setup sentinel node
            sf_free_list_heads[list_index].body.links.next = &sf_free_list_heads[list_index];
            sf_free_list_heads[list_index].body.links.prev = &sf_free_list_heads[list_index];
            list_index++;
        }


        sf_block *heap_pointer = (sf_block *) sf_mem_start();
        size_t *heap_pointer_T = (size_t*)heap_pointer;
        // prolouge skip 24 bytes 

        heap_pointer_T  += 3;  // 3 * 8 = 24

        //mask prologue
        set_block_size_length(heap_pointer, 32);
        set_allocation(heap_pointer, 1);
        copy_header_to_footer(heap_pointer);
        

        heap_pointer_T += 4; // 4 * 8 = 32 go to free list

        //first free block
        heap_pointer = (sf_block *)heap_pointer_T ;
        set_block_size_length(heap_pointer, 1984);

        //add to 7th linked list to the fornt(next to senteinel) store in wildeerneess
        free_list_insert(heap_pointer, 7);
        copy_header_to_footer(heap_pointer);

        // epilogue
        size_t *epilogueP = (size_t *)sf_mem_end();
        epilogueP -= 1;
        sf_block *epilogue = (sf_block *)epilogueP;
        epilogue->header = 0; 
        set_allocation(epilogue, 1);

    }
    int block_found = 0;//If a big enough block is not found in any of the freelists
    sf_block *payload_pointer = NULL;
    int curr_free_index = index_to_store;

    // grow_heap 
    while(block_found == 0)
    {
        curr_free_index = index_to_store;
        while(curr_free_index != 8)
        {
            sf_block *curr_node_free = sf_free_list_heads[curr_free_index].body.links.next;
            while(curr_node_free->header != 0)
            {
                if(bytesNeeded <= get_block_size_length(curr_node_free)) //enough space, split if needed?
                {
                    size_t old_size = get_block_size_length(curr_node_free) ; 
                    size_t new_size = old_size - bytesNeeded; 
                    
                    free_list_remove(curr_node_free); //remove free from freelist
                    //allocate new block
                    set_block_size_length(curr_node_free, bytesNeeded);
                    set_allocation(curr_node_free, 1);
                    copy_header_to_footer(curr_node_free);

                    //return pointer to payload
                    size_t *payload_t = (size_t *) curr_node_free;
                    payload_t += 1;
                    payload_pointer = (sf_block *) payload_t;

                    if(new_size < 32) // "over-allocate" by issuing a block slightly larger than that required
                    {
                        //removed from linked list
                    }
                    else // put new_size into correct free_list index
                    {
                        size_t *curr_node_free_T = (size_t *) curr_node_free;
                        curr_node_free_T += get_block_size_length(curr_node_free) / 8;
                        curr_node_free = (sf_block *) curr_node_free_T;

                        set_block_size_length(curr_node_free, new_size);
                        set_allocation(curr_node_free, 0);
                        
                        //add linked list to the frnt(next to senteinel) for appropiate list
                        if(curr_free_index == 7){
                            free_list_insert(curr_node_free, 7);
                        }
                        else{
                            free_list_insert(curr_node_free, free_list_index(get_block_size_length(curr_node_free)));
                        }
                        copy_header_to_footer(curr_node_free);

                    }

                    block_found = 1;
                    break;

                }
                else
                {
                    curr_node_free = curr_node_free->body.links.next;
                }
            }
            if(block_found == 1)
            {
                break;
            }
            curr_free_index++;
        }
        if(block_found == 1)
        {
            break;
        }
        if(grow_heap(bytesNeeded) == -1){
            sf_errno = ENOMEM;
            return NULL;
        }
    }

   //return pointer to payload
    return payload_pointer ;
}

int grow_heap(size_t bytesNeeded) //2048 bytes, coalesce with whatever is inside the wilderness 
{
    size_t avaliable_bytes = 0; // 2048 - footer
    sf_block *wilderness = sf_free_list_heads[7].body.links.next;
    avaliable_bytes += wilderness->header;

    while(avaliable_bytes < bytesNeeded)
    {
        wilderness = sf_free_list_heads[7].body.links.next; //current wilderness
        

        sf_block *wilderness_new_footer = (sf_block *)sf_mem_end();


        if(sf_mem_grow() == NULL){
            //printf("No more memory \n");
            return -1;
        }
       avaliable_bytes += 2048;


        //set footer of new page
        size_t *epilogueP = (size_t *)sf_mem_end();
        epilogueP -= 1;
        sf_block *epilogue = (sf_block *)epilogueP;
        epilogue->header = 0;
        set_allocation(epilogue, 1);

        //coalasece wilderness with new page

        size_t *wilderness_new_footerP = (size_t *) wilderness_new_footer;
        wilderness_new_footerP -= 1;
        wilderness_new_footer = (sf_block *) wilderness_new_footerP;

        set_block_size_length(wilderness_new_footer, 2048);
        set_allocation(wilderness_new_footer, 0);

        if(wilderness->body.links.next != wilderness) //if there is already a wild block 
        {
            free_list_remove(wilderness);
            free_list_insert(wilderness, free_list_index(get_block_size_length(wilderness)));
        }

        //add to 7th linked list to the fornt(next to senteinel) store in wildeerneess
        copy_header_to_footer(wilderness_new_footer);

        size_t *left_block_p = wilderness_new_footerP - 1;
        sf_block *left_block = (sf_block *) left_block_p; //left footer

        while((get_allocation(left_block) == 0) && (get_block_size_length(left_block) >= 32)){

            left_block_p -= get_block_size_length(left_block) / 8; //go to header of left block - 1
            left_block_p += 1; // 
            left_block = (sf_block *) left_block_p;

            size_t newSize = left_block->header + wilderness_new_footer->header;
            free_list_remove(left_block);
            //allocate new block
            set_block_size_length(left_block, newSize);
            set_allocation(left_block, 0);
            copy_header_to_footer(left_block);

            wilderness_new_footer = left_block;
            wilderness_new_footerP = (size_t *) wilderness_new_footer;

            left_block_p = wilderness_new_footerP  - 1;
            left_block = (sf_block *) left_block_p; //left footer
            
        }

        free_list_insert(wilderness_new_footer , 7);
    }
    return 0;
}

void coalesece(sf_block * block1, sf_block *block2) //assume both blocks point to headers of free blocks
{
    size_t newSize = block1->header + block2->header;
    if(block1->header == 0){
        free_list_remove(block1); //remove free from freelist
    }
    if(block2->header == 0){
        free_list_remove(block2); //remove free from freelist
    }
    
    //allocate new block
    set_block_size_length(block1, newSize);
    set_allocation(block1, 0);
    copy_header_to_footer(block1);
    return;
}

void set_block_size_length(sf_block *pointer, size_t num){ //sets size of sf_block to a # of bits
    pointer->header= num;
    return;
}

size_t get_block_size_length(sf_block *pointer){ //return in bytes
    size_t size = pointer->header;
    size = size >> 5;
    size = size << 5;
    return size;
}
void set_allocation(sf_block *pointer, size_t allo){
    pointer->header = pointer->header >> 5; //remove allocation num
    pointer->header = pointer->header << 5;
    size_t allocation = allo << 4;
    pointer->header = pointer->header | allocation;
    return;

}
int get_allocation(sf_block *pointer){
    size_t size = pointer->header;
    if((size & 16) == 16 )
    {
        return 1;
    }
    else{
        return 0;
    }
    
}
void copy_header_to_footer(sf_block *pointer){
    sf_block *tempPointer = pointer;
    size_t *tempPointerPosition = (size_t *) tempPointer;
    size_t size = get_block_size_length(pointer) / 8;
    tempPointerPosition += size;
    tempPointer = (sf_block *) tempPointerPosition;
    tempPointerPosition -= 1;

    tempPointer = (sf_block *) tempPointerPosition;
    tempPointer->header = pointer->header;
    return;

}

void free_list_insert(sf_block *pointer, int index){

//set pointer links
pointer->body.links.next = sf_free_list_heads[index].body.links.next;
pointer->body.links.prev = &sf_free_list_heads[index];

//set next prev link of next node
sf_free_list_heads[index].body.links.next->body.links.prev = pointer;

//set setenel next to new pointer
sf_free_list_heads[index].body.links.next = pointer;

}

void free_list_remove(sf_block *pointer){
pointer->body.links.prev->body.links.next = pointer->body.links.next;
pointer->body.links.next->body.links.prev = pointer->body.links.prev;

pointer->body.links.next = NULL;
pointer->body.links.prev = NULL;

}

int size_of_block(int size){ //determine the size of the block to be allocated in bytes
    int totalSize = size + 16; // header + fooder = 16 bytes
    if(totalSize % 32 == 0){
        return totalSize;
    }
    else if (totalSize <= 32){
        totalSize = 32;
    }
    else{
        totalSize = totalSize  + 32 - (totalSize % 32);
    }
    return totalSize;
}





void sf_free(void *pp) { //pp = pointer of payload


    if(pp == NULL)
    {
        abort();
    }

    if( (size_t)pp % 32 != 0){
        abort();
    }

    //move pointer to footer
    sf_block *pp_block = (sf_block *)pp;
    size_t *pp_block_p = (size_t *) pp_block;
    pp_block_p -= 1;
    pp_block = (sf_block *) pp_block_p;

    if((get_block_size_length(pp_block) < 32) || (get_block_size_length(pp_block) % 32 != 0 ) || (get_allocation(pp_block) == 0))
    {
        abort();
    }
    
    size_t *heap_start = (size_t *)sf_mem_start();
    heap_start += 3; // beginning of prologue
    size_t *heap_end = (size_t *) sf_mem_end();
    heap_end -= 1;

    sf_block *pp_footer = (sf_block *)pp;
    size_t *pp_footer_P = (size_t *)pp_footer;
    pp_footer_P += get_block_size_length(pp_footer) / 8;
    pp_footer_P -= 1;
    pp_footer = (sf_block *)pp_footer_P;


    if(((size_t)pp < (size_t)heap_start) || ((size_t)pp_footer > (size_t)heap_end))
    {
        abort();
    }
    
    //start freeing
    set_allocation(pp_block, 0);
    copy_header_to_footer(pp_block);

    //free left

    size_t *left_block_p = pp_block_p - 1;
    sf_block *left_block = (sf_block *) left_block_p; //left footer

    while((get_allocation(left_block) == 0) && (get_block_size_length(left_block) >= 32)){

        left_block_p -= get_block_size_length(left_block) / 8; //go to header of left block - 1
        left_block_p += 1; // 
        left_block = (sf_block *) left_block_p;

        size_t newSize = left_block->header + pp_block->header;
        free_list_remove(left_block);
        //allocate new block
        set_block_size_length(left_block, newSize);
        set_allocation(left_block, 0);
        copy_header_to_footer(left_block);

        pp_block = left_block;
        pp_block_p = (size_t *) pp_block;

        left_block_p = pp_block_p - 1;
        left_block = (sf_block *) left_block_p; //left footer
        
    }
    //free right, if last right is epilogue, blcok goes into wildereness 
    
    size_t *right_block_p = pp_block_p + (get_block_size_length(pp_block) / 8);
    sf_block *right_block = (sf_block *) right_block_p; //right footer
    while((get_allocation(right_block) == 0) && (get_block_size_length(right_block) >= 32)){

        size_t newSize = right_block->header + pp_block->header;
        free_list_remove(right_block);
        //allocate new block
        set_block_size_length(pp_block, newSize);
        set_allocation(pp_block, 0);
        //free_list_insert(block1, free_list_index(get_block_size_length(block1)));
        copy_header_to_footer(pp_block);

        right_block_p = pp_block_p + (get_block_size_length(pp_block) / 8);
        right_block = (sf_block *) right_block_p; //left footer
        
    }

    pp_block_p += get_block_size_length(pp_block) / 8;

    sf_block *wild_check = sf_free_list_heads[7].body.links.next;
    size_t * wild_check_p = (size_t *) wild_check;
    size_t wild_check_address = (size_t) wild_check_p;

    if(wild_check->body.links.next == wild_check){ //if it is equal to itself(empty)
        free_list_insert(pp_block, 7);
    }
    else if( (wild_check->body.links.next != wild_check) && ((size_t) pp_block_p > wild_check_address))
    {
        free_list_remove(wild_check);
        free_list_insert(wild_check, free_list_index(get_block_size_length(wild_check)));
        free_list_insert(pp_block, 7);
        
    }
    else if((size_t)heap_end == (size_t)pp_block_p) //if next block is epilogue, insert into wilderness
    {
        free_list_insert(pp_block, 7);
    }
    else
    {
        free_list_insert(pp_block, free_list_index(get_block_size_length(pp_block)));
    }

    return;

}

void *sf_realloc(void *pp, size_t rsize) {

    if(pp == NULL)
    {
        sf_errno = EINVAL;
        abort();
    }

    if( (size_t)pp % 32 != 0){
        sf_errno = EINVAL;
        abort();
    }

    if(rsize == 0)
    {
        sf_free(pp);
        return NULL;
    }

    //move pointer to footer
    sf_block *pp_block = (sf_block *)pp;
    size_t *pp_block_p = (size_t *) pp_block;
    pp_block_p -= 1;
    pp_block = (sf_block *) pp_block_p;

    if((get_block_size_length(pp_block) < 32) || (get_block_size_length(pp_block) % 32 != 0 ) || (get_allocation(pp_block) == 0))
    {
        abort();
    }
    
    size_t *heap_start = (size_t *)sf_mem_start();
    heap_start += 3; // beginning of prologue
    size_t *heap_end = (size_t *) sf_mem_end();
    heap_end -= 1;

    sf_block *pp_footer = (sf_block *)pp;
    size_t *pp_footer_P = (size_t *)pp_footer;
    pp_footer_P += get_block_size_length(pp_footer) / 8;
    pp_footer_P -= 1;
    pp_footer = (sf_block *)pp_footer_P;


    if(((size_t)pp < (size_t)heap_start) || ((size_t)pp_footer > (size_t)heap_end))
    {
        abort();
    }

    if(rsize == 0)
    {
        return NULL;
    }


    if(rsize > get_block_size_length(pp_block)) //larger size
    {
        sf_block *new_malloc = sf_malloc(rsize);
        if(new_malloc == NULL) 
        {
            return NULL;
        }

        memcpy(new_malloc, pp, rsize);
        sf_free(pp);

        return new_malloc;
    }

    else if(rsize < get_block_size_length(pp_block)) // smaller size
    {
        //check if splinter
        size_t new_size = size_of_block(rsize);
        size_t space_freed = get_block_size_length(pp_block) - new_size;

        if(space_freed < 32)
        { //splinter
            return pp;
        }

        size_t * freed_block_p  = pp_block_p + (get_block_size_length(pp_block) / 8);
        freed_block_p -= space_freed / 8;
        sf_block * freed_block = (sf_block *) freed_block_p;

        set_block_size_length(freed_block, space_freed);
        set_allocation(freed_block, 0);
        copy_header_to_footer(freed_block);

        //coalesce if needed
        size_t *right_block_p = freed_block_p + (get_block_size_length(freed_block) / 8);
        sf_block *right_block = (sf_block *) right_block_p; //right footer

        size_t *heap_end = (size_t *) sf_mem_end();
        heap_end -= 1;

        while((get_allocation(right_block) == 0) && (get_block_size_length(right_block) >= 32)){

            size_t newSize = right_block->header + freed_block->header;
            free_list_remove(right_block);
            //allocate new block
            set_block_size_length(freed_block, newSize);
            set_allocation(freed_block, 0);
            copy_header_to_footer(freed_block);

            right_block_p = freed_block_p + (get_block_size_length(freed_block) / 8);
            right_block = (sf_block *) right_block_p; //left footer
            
        }

        freed_block_p += get_block_size_length(freed_block) / 8;
        if((size_t)heap_end == (size_t)freed_block_p) //if next block is epilogue, insert into wilderness
        {
            free_list_insert(freed_block, 7);
        }
        else
        {
            free_list_insert(freed_block, free_list_index(get_block_size_length(freed_block)));
        }


        set_block_size_length(pp_block, new_size);
        set_allocation(pp_block, 1);
        copy_header_to_footer(pp_block);

        pp_block_p += 1;
        pp_block = (sf_block *) pp_block_p;

        return pp_block; // RETURN PAYLOAD
    }

    return NULL;
}

void *sf_memalign(size_t size, size_t align) {
    
    
    if(align < 32){
        sf_errno = EINVAL;
        //printf("Error \n");
        return NULL;
    }

    if(size == 0)
    {
        return NULL;
    }

    size_t power_of_two = align;

    while(power_of_two != 32){
        if(power_of_two % 2 != 0){
            sf_errno = EINVAL;
            //printf("Error \n");
            return NULL;
        }
        power_of_two /= 2;
    }

    //intialize free lists

    int is_heap_init = 0;
    if (sf_mem_end() != sf_mem_start()){
        is_heap_init = 1;
    }

    if(is_heap_init == 0){
        sf_mem_grow();
        //printf("start : %p \n", sf_mem_start());
        // printf("end : %p \n", sf_mem_end());
        int list_index = 0;
        while(list_index < NUM_FREE_LISTS){ // setup sentinel node
            sf_free_list_heads[list_index].body.links.next = &sf_free_list_heads[list_index];
            sf_free_list_heads[list_index].body.links.prev = &sf_free_list_heads[list_index];
            list_index++;
        }

        sf_block *heap_pointer = (sf_block *) sf_mem_start();
        size_t *heap_pointer_T = (size_t*)heap_pointer;
        // prolouge skip 24 bytes 

        heap_pointer_T  += 3;  // 3 * 8 = 24

        //mask prologue
        set_block_size_length(heap_pointer, 32);
        set_allocation(heap_pointer, 1);
        copy_header_to_footer(heap_pointer);
        
        heap_pointer_T += 4; // 4 * 8 = 32 go to free list

        //first free block
        heap_pointer = (sf_block *)heap_pointer_T ;
        set_block_size_length(heap_pointer, 1984);

        //add to 7th linked list to the fornt(next to senteinel) store in wildeerneess
        free_list_insert(heap_pointer, 7);
        copy_header_to_footer(heap_pointer);

        // epilogue
        size_t *epilogueP = (size_t *)sf_mem_end();
        epilogueP -= 1;
        sf_block *epilogue = (sf_block *)epilogueP;
        epilogue->header = 0; 
        set_allocation(epilogue, 1);

    }

    size_t block_size = size + align + 32 ;

    sf_block *new_block = sf_malloc(block_size);
    size_t *new_block_p = (size_t *) new_block;

    sf_block *new_payload = new_block;
    size_t *new_payload_p = (size_t *) new_payload;
    

    size_t new_payload_address = (size_t) new_payload_p;

    if((new_payload_address % align) == 0)
    {
        return new_payload;
    }

    //not aligned
    new_block_p -= 1; // go to footer
    new_block = (sf_block *) new_block_p;

    size_t new_block_size = get_block_size_length(new_block);

    sf_block *left_block = new_block; 

    size_t left_block_size = 0; // bytes

    while(1) //find aligned address
    {
        if(new_payload_address % align == 0)
        {
            if(left_block_size % 32 == 0) // 4 * 8 = 32
            {
                break;
            }
        }
        new_block_p += 1;
        new_block = (sf_block *) new_block_p;
        
        left_block_size += 8;
        new_block_size -= 8;

        new_payload_p += 1;
        new_payload_address = (size_t)new_payload_p;
        new_payload = (sf_block *) new_payload_p;
    }
    
    set_block_size_length(left_block, left_block_size);
    set_allocation(left_block, 0);
    free_list_insert(left_block, free_list_index(get_block_size_length(left_block)));
    copy_header_to_footer(left_block);

    set_block_size_length(new_block,  new_block_size);
    set_allocation(new_block, 1);
    copy_header_to_footer(new_block);

    int min_size = size_of_block(size + 16); //smallest size the new blokcc can be (size + footer and header)
    if(min_size < new_block_size) //split right side
    {
        size_t split_size = new_block_size - min_size;
        set_block_size_length(new_block,  min_size);
        set_allocation(new_block, 1);
        copy_header_to_footer(new_block);

        new_block_p += get_block_size_length(new_block) / 8;
        sf_block *split_block = (sf_block *) new_block_p; 
        size_t *split_block_p = (size_t *) split_block; 
        
        set_block_size_length(split_block, split_size);
        set_allocation(split_block, 0);
        copy_header_to_footer(split_block);

        //attempt to coalsece with right_side
        size_t *right_block_p = split_block_p + (get_block_size_length(split_block) / 8);
        sf_block *right_block = (sf_block *) right_block_p; //right footer

        while((get_allocation(right_block) == 0) && (get_block_size_length(right_block) >= 32)){

            size_t newSize = right_block->header + split_block->header;
            free_list_remove(right_block);
            //allocate new block
            set_block_size_length(split_block, newSize);
            set_allocation(split_block, 0);
            copy_header_to_footer(split_block);

            right_block_p = split_block_p + (get_block_size_length(split_block) / 8);
            right_block = (sf_block *) right_block_p; //left footer
            
        }

        split_block_p += get_block_size_length(split_block) / 8;

        size_t *heap_end = (size_t *) sf_mem_end();
        heap_end -= 1;

        if((size_t)heap_end == (size_t)split_block_p) //if next block is epilogue, insert into wilderness
        {
            free_list_insert(split_block, 7);
        }
        else
        {
            free_list_insert(split_block, free_list_index(get_block_size_length(split_block)));
        }
    }

    if(new_payload == NULL){
        sf_errno = ENOMEM;
        return NULL;
    }

    return new_payload;
}


int free_list_index(int size){ //fibnoachi seuqence
    int index = 0;

    if(size == MIN_BLOCK_SIZE)
    {
        index = 0;
    }
    else if(size == MIN_BLOCK_SIZE * 2 )
    {
        index = 1;
    }
    else if(size == MIN_BLOCK_SIZE * 3 )
    {
        index = 2;
    }
    else if(size <= MIN_BLOCK_SIZE * 5 )
    {
        index = 3;
    }
    else if(size <= MIN_BLOCK_SIZE * 8 )
    {
        index = 4;
    }
    else if(size <= MIN_BLOCK_SIZE * 13 )
    {
        index = 5;
    }
    else  // > 13 * 32
    {
        index = 6;
    }
    
    return index;
}