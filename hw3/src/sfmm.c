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


const int fi_seq[] = {1, 2, 3, 5, 8, 13, 21, 34, 125, 100000};
void *sf_heap_pstart = NULL;
void *sf_heap_pend = NULL;
void *sf_heap_use_end = NULL;
int sf_page_num = 0;

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
    return i;
}
void sf_insert_freelist(int i, void *bp){

    ((sf_block *)bp)->body.links.next = sf_free_list_heads[i].body.links.next;

    ((sf_block *)bp)->body.links.prev = &(sf_free_list_heads[i]);
    sf_free_list_heads[i].body.links.next = bp;
    if(sf_free_list_heads[i].body.links.prev == NULL)
    {
        sf_free_list_heads[i].body.links.prev = bp;
    }
}
bool sf_is_hyblock(void *bp){
    //size_t *hdrp = sf_hdrp(bp);
    size_t *ftrp = sf_ftrp(bp);
    if((void *)ftrp +  HSIZE == sf_heap_use_end){
        return true;
    }
    else
        return false;

}
void sf_segre_storage(void *bp){
    size_t *pheader = sf_hdrp(bp);
    size_b bsize = sf_get_block_size(pheader);
    if(sf_is_hyblock(bp)){
        sf_insert_freelist(9, bp);
        return;
    }
    int i = sf_find_fit_index(bsize);
    sf_insert_freelist(i, bp);
}
void *sf_heap_grow(){
    void *bp = sf_mem_grow();
    sf_page_num ++;
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
size_t sf_acquire_hysize()
{
        void *hybp = sf_free_list_heads[9].body.links.next;
        size_t hysize = sf_get_block_size(sf_hdrp(hybp));
        return hysize;
}
void* sf_find_fit(size_t size){
    void *bp = NULL;
    int i = sf_find_fit_index(size);
    if(i == 9){
        size_t hysize = sf_acquire_hysize();
        if(size > hysize){
            return NULL;
        }

    }

    if(sf_free_list_heads[i].prev_footer >= size){

        while(sf_free_list_heads[i].body.links.next != &(sf_free_list_heads[i])){
            if(sf_get_block_size(sf_hdrp(sf_free_list_heads[i].body.links.next)) >= size){
                bp = sf_free_list_heads[i].body.links.next;
                sf_free_list_heads[i].body.links.next = (sf_free_list_heads[i].body.links.next)->body.links.next;
            }
        }
    }
    else{
        for(i = i+1; i < NUM_FREE_LISTS; i++){
            if(sf_free_list_heads[i].body.links.next != &(sf_free_list_heads[i])){
                bp = sf_free_list_heads[i].body.links.next;
                sf_delete(bp);
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

    if(fsize){
        size_t fheader = sf_pack(0, fsize, 0, 1);
        sf_put((void *)ftrp + MRSIZE, fheader);
        sf_put(raw_ftrp, fheader);
        sf_segre_storage((void *)ftrp);
    }
}
void *sf_get_hybp()
{
    return sf_free_list_heads[9].body.links.next;
}
void *sf_extend_heap(size_t size){
    size_t hysize = sf_acquire_hysize();
    size_t needsize = size - hysize;
    int flag = 0;
    int num_page = needsize / CHUNKSIZE + ((needsize % CHUNKSIZE) ? 1 : 0);
    int i;
    for(i = 0; i < num_page; i++){
        if(sf_page_num > 27){
            flag = 1;
            i--;
            break;
        }
        sf_heap_grow();
    }
    void *hybp = sf_get_hybp();
    void *hdrp = sf_hdrp(hybp);
    
    size_t hyheader = sf_pack(0, hysize + (i) * CHUNKSIZE, 0, sf_get_prv_alloc(hdrp));
    
    sf_put(hdrp, hyheader);
    void *ftrp = sf_ftrp(hybp);
    sf_put(ftrp, hyheader);
    if(flag)
        return NULL;
    else{
        sf_delete(hybp);
        return hybp;
    }
        
}

void my_show_freelists()
{
    for(int i = 0; i < NUM_FREE_LISTS; i++){
        printf("sf_free_list_heads[%d]:  (%p)\n",i,  &sf_free_list_heads[i]);
        sf_block *tmp = sf_free_list_heads[i].body.links.next;
        while(tmp && tmp != &sf_free_list_heads[i]){
            sf_block *bp = sf_free_list_heads[i].body.links.next;
            printf("bp = %p, prv_bp = %p, hdrp = %p, ftrp = %p, header = 0x%016lx, footer = 0x%016lx\n", bp, sf_free_list_heads[i].body.links.prev,sf_hdrp(bp), sf_ftrp(bp), sf_get(sf_hdrp(bp)), sf_get(sf_ftrp(bp)));
            tmp = tmp->body.links.next;
        }
    }
}
void *sf_malloc(size_t size) {
    size_t asize;
    size_t extendsize;
    void *bp;

    if(size == 0)
        return NULL;
    
    if(fcflag){
        sf_heap_init();
        fcflag = 0;
    }
    
    if(size <= 2 * HSIZE)
        asize = 4 * HSIZE;
    else
        asize = (size / (2 * HSIZE) + ((size % (2 * HSIZE)) ? 1 : 0) + 1) * (2 * HSIZE);


    if((bp = sf_find_fit(asize)) != NULL){
        sf_place(bp, asize, size);
        return sf_bp2pp(bp);
    }
    extendsize = MAX(asize, CHUNKSIZE);
    if((bp = sf_extend_heap(extendsize)) == NULL){
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
        void *bp = sf_pp2bp(pp);
        void *hdrp = sf_hdrp(bp);
        size_t size = sf_get_block_size(hdrp);
        size_t arsize;
        if(rsize <= 2 * HSIZE)
            arsize = 4 * HSIZE;
        else
            arsize = (rsize / (2 * HSIZE) + ((rsize % (2 * HSIZE)) ? 1 : 0) + 1) * (2 * HSIZE);
        if(arsize > size){
            void *new_ptr = sf_malloc(rsize);
            if(new_ptr != NULL){
                unsigned long * lt = pp - 8;  // acquire header address
                int payload_size = (*lt) >> 32;
                int old_size = (payload_size / 16 + (payload_size % 16) ? 1 : 0 ) * 16;

                memcpy(new_ptr, pp, old_size < rsize ? old_size:rsize);
                sf_free(pp);
            }
            else{
                sf_errno = ENOMEM;
            }
            return new_ptr;

        }
        else if(arsize <= size && arsize > (size - M)){
            return pp;
        }
        else{
            size_t fsize = size - arsize;
            size_t header = sf_pack(rsize, arsize, 1, sf_get_prv_alloc(hdrp));
            sf_put(hdrp, header);
            void *ftrp = sf_ftrp(bp);
            sf_put(ftrp, header);
            size_t fheader = sf_pack(0, fsize, 0, 1);
            void *fbp = ftrp;
            void *fhdrp = sf_hdrp(fbp);
            sf_put(fhdrp, fheader);
            void *ffrtp = sf_ftrp(fbp);
            sf_put(ffrtp, header);
            sf_free(sf_bp2pp(fbp));
            return pp;
        }




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
