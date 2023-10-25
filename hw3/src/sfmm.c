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
#include "mysfmm.h"
int fcflag = 1;  // first call sf_mem_grow


/*agreement: 
        bp : block point, the point is the address of block, point to prv_footer
        pp : payload point, the point is the address of malloc, point to payload
*/


const int fi_seq[] = {1, 2, 3, 5, 8, 13, 21, 34, 132, 89 ,144};
void *sf_heap_pstart = NULL;
void *sf_heap_pend = NULL;
void *sf_heap_use_end = NULL;

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
    return (size_t *)(bp + HSIZE);

}
size_t *sf_ftrp(void *bp)
{
    return (size_t *)(bp + sf_get_block_size(sf_hdrp(bp)));
}
void *sf_next_blkp(void *bp)
{
    if(bp + sf_get_block_size(sf_hdrp(bp)) == sf_heap_pend)
        return NULL;
    else
        return (bp + sf_get_block_size(sf_hdrp(bp)));
}
void *sf_prev_blkp(void *bp)
{
    if(bp == sf_heap_pstart)
        return NULL;
    else
        return (bp - sf_get_block_size(((size_t *)bp)) );
}

/* According to the acquirement of HW3, design some function */

void *sf_pp2bp(void *pp)
{
    return pp - 2 * HSIZE;
}
void *sf_bp2pp(void *bp)
{
    return bp + 2 * HSIZE;
}
int sf_get_nm(size_b bsize)
{
    return bsize / M + ((bsize % M) ? 1 : 0);
}

int sf_find_fit_index(size_b bsize){
    int nm = sf_get_nm(bsize);
    int i;
    for(i = 0; i < NUM_FREE_LISTS_INSERT; i++){
        if( nm <= fi_seq[i])
            break;
    }
    if(i == 9)
        i = 8;
    return i;
}
void sf_insert_freelist(int i, void *bp){
    //size_t bsize = sf_get_block_size(sf_hdrp(bp) ) ;
    ((sf_block *)bp)->body.links.next = sf_free_list_heads[i].body.links.next;

    ((sf_block *)bp)->body.links.prev = &(sf_free_list_heads[i]);
    sf_free_list_heads[i].body.links.next = bp;
    if(sf_free_list_heads[i].body.links.prev == NULL)
    {
        sf_free_list_heads[i].body.links.prev = bp;
    }
    
    //("插入大小：size = %ld\n", bsize);
    //printf("%s  %d   hdrp = %p, ftrp = %p, header = 0x%016lx, footer = 0x%016lx\n",__func__, __LINE__,sf_hdrp(bp), sf_ftrp(bp), sf_get(sf_hdrp(bp)), sf_get(sf_ftrp(bp)));
    /* sf_free_list_heads[i].prev_footer is the max block size of sf_free_list_heads[i] */
    // if(bsize > sf_free_list_heads[i].prev_footer){
    //     sf_free_list_heads[i].prev_footer = bsize;
    // }
}

void sf_segre_storage(void *bp){
    size_t *pheader = sf_hdrp(bp);
    size_b bsize = sf_get_block_size(pheader);
    int i = sf_find_fit_index(bsize);
    //printf("开始插入链表  i = %d\n", i);
    sf_insert_freelist(i, bp);
}
void *sf_heap_grow(){
    void *bp = sf_mem_grow();
    if(fcflag){
        sf_heap_pstart = bp;
        sf_heap_pend = bp + PAGE_SZ;
        sf_heap_use_end = sf_heap_pend - MRSIZE - M;

    }
    else{
        sf_heap_pend = bp + PAGE_SZ;
        sf_heap_use_end = sf_heap_pend - MRSIZE - M;
    }
    return bp;
}
void sf_heap_init(){

    for(int i = 0; i < NUM_FREE_LISTS ;i++)
    {
        sf_free_list_heads[i].body.links.next = &(sf_free_list_heads[i]);
    }
    void *bp = sf_heap_grow();
    size_t *pheader = (size_t *)(bp + HSIZE);
    size_t size = CHUNKSIZE - 2 * MRSIZE - M;
    size_t header = sf_pack(0, size, 0, 0);
    //header |= (1 << 1);  // set last free block flag

    
    sf_put(pheader, header);
    void *pfooter = sf_ftrp(bp);
    sf_put(pfooter, header);
    ((sf_block *)bp)->body.links.next = NULL;
    ((sf_block *)bp)->body.links.prev = NULL;


    // // 设置最后一页
    // sf_block *last_bp = sf_heap_use_end;
    // last_bp->body.links.next = &sf_free_list_heads[9];
    // last_bp->body.links.prev = &sf_free_list_heads[9];
    // sf_free_list_heads[9].body.links.next = last_bp;
    // sf_free_list_heads[9].body.links.prev = last_bp;


    sf_segre_storage(bp);
}
void *sf_fb_merge(void *lbp, void *hbp)
{
    size_t lsize = sf_get_block_size(sf_hdrp(lbp));
    size_t hsize = sf_get_block_size(sf_hdrp(hbp));
    size_t size = lsize + hsize;
    size_t header = sf_pack(0, size, 0, sf_get_prv_alloc(sf_hdrp(lbp)));
    sf_put( sf_hdrp(lbp) ,header);
    sf_put(sf_ftrp(hbp), header);
    return lbp;
}
void sf_delete(void *bp)
{
    sf_block *prev = ((sf_block *)bp)->body.links.prev;
    sf_block *next = ((sf_block *)bp)->body.links.next;

    if(next){
        next->body.links.prev = prev;
    }
    if(prev){
        prev->body.links.next = next;
    }

}
void* sf_find_fit(size_t size){
    // first-fit search
    void *bp = NULL;
    // find index of sf_free_list_heads
    int i = sf_find_fit_index(size);
    //printf("查找合适的list， i= %d\n", i);

    if(sf_free_list_heads[i].prev_footer >= size){

        while(sf_free_list_heads[i].body.links.next != &(sf_free_list_heads[i])){
            if(sf_get_block_size(sf_hdrp(sf_free_list_heads[i].body.links.next)) >= size){
                bp = sf_free_list_heads[i].body.links.next;
                sf_free_list_heads[i].body.links.next = (sf_free_list_heads[i].body.links.next)->body.links.next;
            }
        }
    }
    else{
        for(i = i+1; i < NUM_FREE_LISTS_INSERT; i++){
            if(sf_free_list_heads[i].body.links.next != &(sf_free_list_heads[i])){
                bp = sf_free_list_heads[i].body.links.next;
                sf_delete(bp);
                //printf("在其它块找到了可以分配的内存 i = %d\n", i);
                break;
            }
        }
    }
    return bp;
}

void sf_place(sf_block* bp, size_t asize, size_t size)
{
    size_t *raw_hdrp = sf_hdrp(bp);
    size_t *raw_ftrp = sf_ftrp(bp);
    
    size_t raw_bsize = sf_get_block_size(raw_hdrp);
    size_t fsize = raw_bsize - asize;

    /* if free size < M, all block mem alloc asize */
    if(fsize < M){
        asize = raw_bsize;
        fsize = 0;
    }
    size_t header = sf_pack(size, asize, 1, sf_get_prv_alloc(raw_hdrp));
    size_t *hdrp = sf_hdrp(bp);
    sf_put(hdrp, header) ;
    size_t *ftrp = sf_ftrp(bp);
    sf_put(ftrp, header);

    /* 分离free块 */
    if(fsize){
        size_t fheader = sf_pack(0, fsize, 0, 1);
        sf_put((void *)ftrp + MRSIZE, fheader);
        sf_put(raw_ftrp, fheader);
        //printf("%s %d, fhead = 0x%016lx, asize = %ld, fsize = %ld, raw_bsize = %ld\n", __func__, __LINE__, fheader, asize, fsize, raw_bsize);
        sf_segre_storage((void *)ftrp);
    }
}

void *sf_extend_heap(size_t size){
    int num_page = size / CHUNKSIZE + ((size / CHUNKSIZE) ? 1 : 0);
    void *p = sf_heap_grow();
    for(int i = 1; i < num_page; i++){
        sf_heap_grow();
    }
    return p;
}


void *sf_malloc(size_t size) {
    size_t asize;
    size_t extendsize;
    void *bp;

    if(size == 0)
        return NULL;
    
    if(fcflag){
        sf_heap_init();
        //printf("首次缺页\n");
        fcflag = 0;
    }
    
    if(size <= 2 * HSIZE)
        asize = 4 * HSIZE;
    else
        asize = (size / (2 * HSIZE) + ((size % (2 * HSIZE)) ? 1 : 0) + 1) * (2 * HSIZE);


    if((bp = sf_find_fit(asize)) != NULL){
        //printf("malloc  分配到块 %p, asize = %ld\n", bp, asize);
        sf_place(bp, asize, size);
        return sf_bp2pp(bp);
    }
    
    extendsize = MAX(asize, CHUNKSIZE);
    if((bp = sf_extend_heap(extendsize / WSIZE)) == NULL){
        sf_errno = ENOMEM;
        return NULL;
    }
    sf_place(bp, asize, size);
    return sf_bp2pp(bp);
}

void *sf_coalesce(void *bp)
{
    void *next_blkp = sf_next_blkp(bp);
    void *prev_blkp = sf_prev_blkp(bp);

    if(next_blkp != NULL && sf_get_alloc(sf_hdrp(next_blkp)) == 0){
        sf_delete(next_blkp);
        bp = sf_fb_merge(bp, next_blkp);
    }
    if(prev_blkp != NULL && sf_get_alloc(sf_hdrp(prev_blkp)) == 0){
        sf_delete(prev_blkp);
        bp = sf_fb_merge(prev_blkp, bp);
    }
    sf_segre_storage(bp);
    return bp;
}
void sf_free(void *pp) {
    void *bp = sf_pp2bp(pp);
    size_t size = sf_get_block_size(sf_hdrp(bp));
    bool prv_alloc = sf_get_prv_alloc(sf_hdrp(bp));
    size_t header = sf_pack(0, size, 0, prv_alloc);
    size_t *hdrp = sf_hdrp(bp);
    size_t *ftrp = sf_ftrp(bp);
    sf_put(hdrp, header);
    sf_put(ftrp, header);
    ((sf_block *)bp)->body.links.next = sf_next_blkp(bp);
    ((sf_block *)bp)->body.links.prev = sf_prev_blkp(bp);
    //printf("开始free， bp = %p, size = %ld\n", bp, size);
    sf_coalesce(bp);
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
