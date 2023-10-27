#ifndef __MYSFMM_H__
#define __MYSFMM_H__
#include <stdio.h>
#include "sfmm.h"
// free list : 1 2 3 5 8 13 21 34 55 89 144
// M  32

// This assignment basic constants and macros
#define WSIZE 2  // word size  2
#define DSIZE 4  // double word size 4
#define QSIZE 8  // quadword size 8
#define BHSIZE (2 * WSIZE)  // Block header size is 2 word size, equals 4 byte
#define HSIZE (4 * WSIZE)  // Header size is 4 word size, equals 8 byte
#define MRSIZE 8            // In this hw, memory row size is 8 byte

#define CHUNKSIZE (1 << 12)  // extend heap - a page size  4096  4096/32 = 128
#define MAX(x, y) ((x) > y ? (x) : (y))
#define M 32
#define NUM_FI_SEQ 11
#define NUM_FREE_LISTS_INSERT 9
typedef unsigned int size_b; 
size_t sf_pack(size_b psize, size_b bsize, bool alloc, bool prv_alloc);
size_t sf_get(void *p);
void sf_put(void *p, size_t val);
size_b sf_get_payload_size(size_t *v);
size_b sf_get_block_size(size_t *v);
bool sf_get_alloc(size_t *v);
bool sf_get_prv_alloc(size_t *v);
size_t *sf_hdrp(void *bp);
size_t *sf_ftrp(void *bp);
void *sf_next_blkp(void *bp);
void *sf_prev_blkp(void *bp);
void my_show_freelists();
#endif