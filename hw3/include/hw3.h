#ifndef HW3_H
#define HW3_H

void* find_fit(size_t asize);

void place(sf_free_header* bp, size_t block_size, size_t padding, size_t reqSize);

void* coalesce(sf_free_header* bp);

double calcPMU();

#endif
