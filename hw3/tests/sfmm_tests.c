#include <criterion/criterion.h>
#include <errno.h>
#include <signal.h>
#include "debug.h"
#include "sfmm.h"
#include "mysfmm.h"
#define TEST_TIMEOUT 15

/*
 * Assert the total number of free blocks of a specified size.
 * If size == 0, then assert the total number of all free blocks.
 */
void assert_free_block_count(size_t size, int count) {
    int cnt = 0;
    for(int i = 0; i < NUM_FREE_LISTS; i++) {
	sf_block *bp = sf_free_list_heads[i].body.links.next;
	while(bp != &sf_free_list_heads[i]) {
	    if(size == 0 || size == (bp->header & ~0xf))
		cnt++;
	    bp = bp->body.links.next;
	}
    }
    if(size == 0) {
	cr_assert_eq(cnt, count, "Wrong number of free blocks (exp=%d, found=%d)",
		     count, cnt);
    } else {
	cr_assert_eq(cnt, count, "Wrong number of free blocks of size %ld (exp=%d, found=%d)",
		     size, count, cnt);
    }
}

/*
 * Assert that the free list with a specified index has the specified number of
 * blocks in it.
 */
void assert_free_list_size(int index, int size) {
    int cnt = 0;
    sf_block *bp = sf_free_list_heads[index].body.links.next;
    while(bp != &sf_free_list_heads[index]) {
	cnt++;
	bp = bp->body.links.next;
    }
    cr_assert_eq(cnt, size, "Free list %d has wrong number of free blocks (exp=%d, found=%d)",
		 index, size, cnt);
}

Test(sfmm_basecode_suite, malloc_an_int, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	int *x = sf_malloc(sizeof(int));

	cr_assert_not_null(x, "x is NULL!");

	*x = 4;

	cr_assert(*x == 4, "sf_malloc failed to give proper space for an int!");

	assert_free_block_count(0, 1);
	assert_free_block_count(4016, 1);
	assert_free_list_size(9, 1);

	cr_assert(sf_errno == 0, "sf_errno is not zero!");
	cr_assert(sf_mem_start() + PAGE_SZ == sf_mem_end(), "Allocated more than necessary!");
}

Test(sfmm_basecode_suite, malloc_four_pages, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	// We want to allocate up to exactly four pages.
	void *x = sf_malloc(16384 - 48 - (sizeof(sf_header) + sizeof(sf_footer)));

	cr_assert_not_null(x, "x is NULL!");
	assert_free_block_count(0, 0);
	cr_assert(sf_errno == 0, "sf_errno is not 0!");
}

Test(sfmm_basecode_suite, malloc_too_large, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	void *x = sf_malloc(PAGE_SZ << 16);

	cr_assert_null(x, "x is not NULL!");
	assert_free_block_count(0, 1);
	assert_free_block_count(110544, 1);
	cr_assert(sf_errno == ENOMEM, "sf_errno is not ENOMEM!");
}
// PASS
Test(sfmm_basecode_suite, free_no_coalesce, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	/* void *x = */ sf_malloc(8);
	void *y = sf_malloc(200);
	/* void *z = */ sf_malloc(1);

	sf_free(y);

	assert_free_block_count(0, 2);
	assert_free_block_count(224, 1);
	assert_free_block_count(3760, 1);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}
// PASS
Test(sfmm_basecode_suite, free_coalesce, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	/* void *w = */ sf_malloc(8);
	void *x = sf_malloc(200);
	void *y = sf_malloc(300);
	/* void *z = */ sf_malloc(4);

	sf_free(y);
	sf_free(x);

	assert_free_block_count(0, 2);
	assert_free_block_count(544, 1);
	assert_free_block_count(3440, 1);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sfmm_basecode_suite, freelist, .timeout = TEST_TIMEOUT) {
	void *u = sf_malloc(200);
	/* void *v = */ sf_malloc(300);
	void *w = sf_malloc(200);
	/* void *x = */ sf_malloc(500);
	void *y = sf_malloc(200);
	/* void *z = */ sf_malloc(700);

	sf_free(u);
	sf_free(w);
	sf_free(y);

	assert_free_block_count(0, 4);
	assert_free_block_count(224, 3);
	assert_free_block_count(1808, 1);
	assert_free_list_size(4, 3);
	assert_free_list_size(9, 1);

	// First block in list should be the most recently freed block.
	int i = 4;
	sf_block *bp = sf_free_list_heads[i].body.links.next;
	cr_assert_eq(bp, (char *)y - 2*sizeof(sf_header),
		     "Wrong first block in free list %d: (found=%p, exp=%p)!",
                     i, bp, (char *)y - 2*sizeof(sf_header));
}

Test(sfmm_basecode_suite, realloc_larger_block, .timeout = TEST_TIMEOUT) {
	void *x = sf_malloc(sizeof(int));
	/* void *y = */ sf_malloc(10);
	x = sf_realloc(x, sizeof(int) * 20);

	cr_assert_not_null(x, "x is NULL!");
	sf_block *bp = (sf_block *)((char *)x - 2*sizeof(sf_header));
	cr_assert(bp->header & 0x8, "Allocated bit is not set!");
	cr_assert((bp->header & ~0xf & 0xffff) == 96,
		  "Realloc'ed block size not what was expected (found=%lu, exp=%lu)!",
		  bp->header & ~0xf, 96);

	assert_free_block_count(0, 2);
	assert_free_block_count(32, 1);
	assert_free_block_count(3888, 1);
}

Test(sfmm_basecode_suite, realloc_smaller_block_splinter, .timeout = TEST_TIMEOUT) {
	void *x = sf_malloc(sizeof(int) * 20);
	void *y = sf_realloc(x, sizeof(int) * 16);

	cr_assert_not_null(y, "y is NULL!");
	cr_assert(x == y, "Payload addresses are different!");

	sf_block *bp = (sf_block *)((char*)y - 2*sizeof(sf_header));
	cr_assert(bp->header & 0x8, "Allocated bit is not set!");
	cr_assert((bp->header & ~0xf & 0xffff) == 96,
		  "Block size not what was expected (found=%lu, exp=%lu)!",
		  bp->header & ~0xf & 0xffff, 96);

	// There should be only one free block of size 3952.
	assert_free_block_count(0, 1);
	assert_free_block_count(3952, 1);
}

Test(sfmm_basecode_suite, realloc_smaller_block_free_block, .timeout = TEST_TIMEOUT) {
	void *x = sf_malloc(sizeof(double) * 8);
	void *y = sf_realloc(x, sizeof(int));

	cr_assert_not_null(y, "y is NULL!");

	sf_block *bp = (sf_block *)((char*)y - 2*sizeof(sf_header));
	cr_assert(bp->header & 0x8, "Allocated bit is not set!");
	cr_assert((bp->header & ~0xf & 0xffff) == 32,
		  "Realloc'ed block size not what was expected (found=%lu, exp=%lu)!",
		  bp->header & ~0xf & 0xffff, 32);

	// After realloc'ing x, we can return a block of size 32 to the freelist.
	// This block will go into the freelist and be coalesced.
	assert_free_block_count(0, 1);
	assert_free_block_count(4016, 1);
}

//############################################
//STUDENT UNIT TESTS SHOULD BE WRITTEN BELOW
//DO NOT DELETE THESE COMMENTS
//############################################

//Test(sfmm_student_suite, student_test_1, .timeout = TEST_TIMEOUT) {
//}

#include "mysfmm.h"
Test(sfmm_student_suite, student_test_payload_size, .timeout = TEST_TIMEOUT) {
	void *x = sf_malloc(sizeof(double) * 8);

	cr_assert_not_null(x, "x is NULL!");

	sf_block *bp = (sf_block *)((char*)x - 2*sizeof(sf_header));
	size_b x_bs = sf_get_payload_size((size_t *)bp);

	void *y = sf_realloc(x, sizeof(double) * 1024);

	bp = (sf_block *)((char*)y - 2*sizeof(sf_header));
	size_b y_bs = sf_get_payload_size((size_t *)bp);

	cr_assert_eq(x_bs, 0, "Wrong number of payload (exp=%d, found=%d)",
		     0, x_bs);
	cr_assert_eq(y_bs, 0, "Wrong number of payload (exp=%d, found=%d)",
		     0, y_bs);
}


Test(sfmm_student_suite, student_test_block_size, .timeout = TEST_TIMEOUT) {
	void *x = sf_malloc(sizeof(double) * 8);

	cr_assert_not_null(x, "x is NULL!");

	sf_block *bp = (sf_block *)((char*)x - 2*sizeof(sf_header));
	size_b x_bs = sf_get_block_size((size_t *)bp);

	void *y = sf_realloc(x, sizeof(double) * 1024);

	bp = (sf_block *)((char*)y - 2*sizeof(sf_header));
	size_b y_bs = sf_get_block_size((size_t *)bp);

	cr_assert_eq(x_bs, 0, "Wrong number of free blocks (exp=%d, found=%d)",
		     0, x_bs);
	cr_assert_eq(y_bs, 80, "Wrong number of free blocks (exp=%d, found=%d)",
		     80, y_bs);
}


Test(sfmm_student_suite, student_test_3, .timeout = TEST_TIMEOUT) {
	size_t r = sf_pack(256, 1024, false, true);

	cr_assert_eq(r, 0x0000010000000404, "Wrong number of SF_BLOCK (exp=0x%016lx, found=0x%016lx)",
		     0x0000010000000404, r);

	r = sf_pack(0, 0, true, true);

	cr_assert_eq(r, 0xC, "Wrong number of SF_BLOCK (exp=0x%016lx, found=0x%016lx)",
		     0xC, r);

	r = sf_pack(1, 0, false, false);

	cr_assert_eq(r, 0x0000000100000000, "Wrong number of SF_BLOCK (exp=0x%016lx, found=0x%016lx)",
		     0x0000000100000000, r);

	r = sf_pack(256, 1024, true, false);

	cr_assert_eq(r, 0x0000010000000408, "Wrong number of SF_BLOCK (exp=0x%016lx, found=0x%016lx)",
		     0x0000010000000408, r);
}


Test(sfmm_student_suite, student_test_4, .timeout = TEST_TIMEOUT) {
	void *x = sf_malloc(sizeof(double) * 8);

	cr_assert_not_null(x, "x is NULL!");

	sf_block *bp = (sf_block *)((char*)x - 2*sizeof(sf_header));
	bool a = sf_get_alloc((size_t *)bp);
	bool p = sf_get_prv_alloc((size_t *)bp);

	cr_assert(!a,  "ALLOC FLAG is TRUE");
	cr_assert(!p,  "PREV ALLOC FLAG is TRUE");

}


Test(sfmm_student_suite, student_test_5, .timeout = TEST_TIMEOUT) {
	void *x = sf_malloc(sizeof(double) * 1024);

	cr_assert_not_null(x, "x is NULL!");

	size_t* head_address = sf_hdrp(x);
	size_t* foot_address = sf_ftrp(x);



	void *y = sf_realloc(x, sizeof(double) * 256);

	head_address = sf_hdrp(y);
	foot_address = sf_ftrp(y);
	cr_assert_not_null(y, "x is NULL!");

	size_t bytes = (char*)foot_address - (char*)head_address;
	cr_assert(bytes, "bytes zero");

// 	cr_assert_eq((char*)foot_address - (char*)head_address, 0, "Wrong head address (exp=%016lx, found=%016lx)",
// 		     0, (char*)foot_address - (char*)head_address);
// 	cr_assert_eq((char*)foot_address - (char*)head_address, 80, "Wrong foot address  (exp=%016lx, found=%016lx)",
// 		     80, (char*)foot_address - (char*)head_address);
}

