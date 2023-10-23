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
// free list : 1 2 3 5 8 13 21 34 55 89 144
// M  32

// This assignment basic constants and macros
#define WSIZE 2  // word size  2
#define DSIZE 4  // double word size 4
#define QSIZE 8  // quadword size 8
#define BHSIZE (2 * WSIZE)  // Block header size is 2 word size, equals 4 byte
#define HSIZE (4 * WSIZE)  // Header size is 4 word size, equals 8 byte

#define CHUNKSIZE (1 << 12)  // extend heap - a page size  4096  4096/32 = 128
#define MAX(x, y) ((x) > y ? (x) : (y))
#define M 32
#define N_FI_SEQ 10


typedef unsigned int size_b; 
const int fi_seq[] = {1, 2, 3, 5, 8, 13, 21, 34, 55, 89 ,144};

/*According to the requirements of HW3, rewrite section 9.9.12.2 in CSAPP.*/

size_t sf_pack(size_b psize, size_b bsize, bool alloc, bool prv_alloc)
{
    size_t res = 0;
    res |= ((size_t)psize << 32);
    res |= bsize;
    res |= (alloc << 3);
    res |= (prv_alloc << 2);
    return res;
}
/*Read and write a DWORD at address p*/
size_t sf_get(void *p)
{
    return *(size_t *)(p);
}
void sf_put(void *p, size_t val)
{
    *(size_t *)(p) = val;
}
/*Given block ptr bp, get its payload_size and block_size*/
size_b sf_get_payload_size(size_t *v)
{
    return *v >> 32;
}
size_b sf_get_block_size(size_t *v)
{
    return *v & 0x0000fff0;
}
/*Given block ptr bp, get flag of its alloc and prv alloc*/
bool sf_get_alloc(size_t *v)
{
    return *v & (1 << 3);
}
bool sf_get_prv_alloc(size_t *v)
{
    return *v & (1 << 2);
}
/* Given block ptr bp, computer address of its header and footer*/
size_t *sf_hdrp(void *bp)
{
    return (size_t *)(bp - HSIZE);

}
size_t *sf_ftrp(void *bp)
{
    return (size_t *)(bp + sf_get_block_size(sf_hdrp(bp)) - 2 * HSIZE);
}

void* sf_find_fit(size_t size){
    // first-fit search
    void *bp = NULL;
    int i; 
    // find index of sf_free_list_heads
    int nm = size / M + ((size % M) ? 1 : 0 ) ;
    for(i = 0; i < N_FI_SEQ; i++){
        if(nm < fi_seq[i]){
            break;
        }
    }
    
    /* Determine if there is a block that satisfies the condition */
    if(sf_free_list_heads[i].body.links.next){
        bp = &sf_free_list_heads[i];
    }
    else{
        bp = NULL;
    }
    return bp;
}

void sf_place(sf_block* bp, size_t asize, size_t size)
{
    sf_block* bp_header = bp->body.links.next;
    bp->body.links.next = bp_header->body.links.next;
    bp_header->header = size << 32;
    bp_header->header |= asize << 4;
    bp_header->header |= 1 << 3;
    size_t* bp_footer = (size_t*)((size_t)bp_header + 8 + size);
    *bp_footer = size << 32;
    *bp_footer |= asize << 4;
    *bp_footer |= 1 << 3;
    //size_t* bp_next_header = bp_footer + 8;
}

void *sf_extend_heap(size_t size){
    int num_page = size / CHUNKSIZE + ((size / CHUNKSIZE) ? 1 : 0);
    void *p = sf_mem_grow();
    for(int i = 1; i < num_page; i++){
        sf_mem_grow();
    }
    return p;
}

void *sf_malloc(size_t size) {
    size_t asize;
    size_t extendsize;
    void *bp;

    if(size == 0)
        return NULL;
    
    if(size <= 2 * QSIZE)
        asize = 4 * QSIZE;
    else
        asize = size / QSIZE + ((size % QSIZE) ? 2 : 0) + 2;

    if((bp = sf_find_fit(asize)) != NULL){
        sf_place(bp, asize, size);
        return bp + 2 * QSIZE;
    }

    extendsize = MAX(asize, CHUNKSIZE);
    if((bp = sf_extend_heap(extendsize / WSIZE)) == NULL){
        sf_errno = ENOMEM;
        return NULL;
    }
    sf_place(bp, asize, size);
    return bp + 2 * QSIZE;
}



void *sf_coalesce(void *bp)
{
    return NULL;
}
void sf_free(void *pp) {
    size_t size = sf_get_block_size(sf_hdrp(pp));
    bool prv_alloc = sf_get_prv_alloc(sf_hdrp(pp));
    sf_put(sf_hdrp(pp), sf_pack(0, size, 0, prv_alloc));
    sf_coalesce(pp);
}


void *sf_realloc(void *pp, size_t rsize) {
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
