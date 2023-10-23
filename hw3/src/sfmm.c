/**
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "debug.h"
#include "sfmm.h"

void * sf_find_block(size_t size){
    // 从sf_free_list_heads查找有无合适的内存块
    for(int i = 0; i < NUM_FREE_LISTS; i++){
        
    }
    return NULL;

}

void *sf_malloc(size_t size) {
    // To be implemented.
    /*
    if(size == 0){
        return NULL;
    }
    int page_num = size / PAGE_SZ  + ((size % PAGE_SZ) ? 1 : 0);  // 计算需要的页数
    
    //void *p = NULL;



    for(int i = 0; i < page_num; i++){
        //p = sf_mem_grow();
        for(int i = 0; i < NUM_FREE_LISTS; i++){
            if(sf_free_list_heads[i].header == 0){


            }
        }
    }
    
    */
    void *p = sf_mem_grow();

    long *lt = p+8;
    // header
    *lt = 0;
    (*lt) |= size << 32; // payload_size
    (*lt) |= ((size / 16 + (size % 16) ? 1 : 0 ) * 16 + 16) << 4;  // block_ssize
    (*lt) |= 1 << 3;  // alloc
    (*lt) |= 0 << 2;  // prv allock

    lt = p + 16 + (size / 16 + (size % 16) ? 1 : 0 ) * 16 ;

    // footer
    *lt = 0;
    (*lt) |= size << 32; // payload_size
    (*lt) |= ((size / 16 + (size % 16) ? 1 : 0 ) * 16 + 16) << 4;  // block_ssize
    (*lt) |= 1 << 3;  // alloc
    (*lt) |= 0 << 2;  // prv allock

    return p+16;


    
}

void sf_free(void *pp) {
    // To be implemented.
    long* lt = pp - 8;  // acquire header address
    (*lt) |= 0 << 3;  // alloc
    int payload_size = (*lt) >> 32;
    int size = (payload_size / 16 + (payload_size % 16) ? 1 : 0 ) * 16;
    lt = (long *)((long int)pp + size);
    (*lt) |= 0 << 3;  // alloc
}

void *sf_realloc(void *pp, size_t rsize) {
    // To be implemented.
    if(pp == NULL){
        return sf_malloc(rsize);
    }
    else if (rsize == 0){
        sf_free(pp);
        return NULL;
    }
    else{
        void *new_ptr = sf_malloc(rsize);
        if(new_ptr != NULL){
            unsigned long * lt = pp - 8;  // acquire header address
            int payload_size = (*lt) >> 32;
            int old_size = (payload_size / 16 + (payload_size % 16) ? 1 : 0 ) * 16;

            memcpy(new_ptr, pp, old_size < rsize ? old_size:rsize);
            free(pp);
        }
        else{
            sf_errno = ENOMEM;
        }
        return new_ptr;
    }
}

double sf_fragmentation() {
    // To be implemented.
    abort();
}

double sf_utilization() {
    // To be implemented.
    abort();
}
