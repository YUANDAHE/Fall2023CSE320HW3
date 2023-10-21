/**
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "sfmm.h"
typedef struct Block {
    size_t size;
    struct Block* next;
} Block;

static Block* g_head = NULL;

void *sf_malloc(size_t size) {
    // To be implemented.
        if (size == 0) {
        return NULL;  
    }

    // 在链表中查找合适的块
    Block* prev = NULL;
    Block* curr = g_head;

    while (curr != NULL) {
        if (curr->size >= size) {

            if (curr->size > size + sizeof(Block)) {
                Block* newBlock = (Block*)((char*)curr + sizeof(Block) + size);
                newBlock->size = curr->size - size - sizeof(Block);
                newBlock->next = curr->next;
                curr->size = size;
                curr->next = newBlock;
            }
            if (prev != NULL) {
                prev->next = curr->next;
            } else {
                g_head = curr->next;
            }
            return (void*)(curr + 1);
        }
        prev = curr;
        curr = curr->next;
    }
    size_t totalSize = size + sizeof(Block);
    curr = (Block*)sf_sbrk(totalSize);
    if (curr == (void*)-1) {
        return NULL;
    }
    curr->size = size;
    curr->next = NULL;

    return (void*)(curr + 1);
}

void sf_free(void *pp) {
    // To be implemented.
     if (pp == NULL) {
        return; 
    }

    Block* block = (Block*)pp - 1;

    Block* prev = NULL;
    Block* curr = g_head;

    while (curr != NULL && curr < block) {
        prev = curr;
        curr = curr->next;
    }

    if (prev == NULL) {
        block->next = g_head;
        g_head = block;
    } else {
        block->next = prev->next;
        prev->next = block;
    }

    curr = g_head;
    while (curr != NULL && curr->next != NULL) {
        if ((char*)curr + curr->size + sizeof(Block) == (char*)curr->next) {
            curr->size += curr->next->size + sizeof(Block);
            curr->next = curr->next->next;
        }
        curr = curr->next;
    }
}

void *sf_realloc(void *pp, size_t rsize) {
    // To be implemented.
        if (rsize == 0) {
        free(pp);
        return NULL;
    }

    if (pp == NULL) {
        // 如果原指针为空，等同于 malloc
        return malloc(rsize);
    }

    // alloc new mem block
    void* new_ptr = malloc(rsize);

    if (new_ptr == NULL) {
        return pp;
    }

    size_t copy_size = rsize;
    if (copy_size > *((size_t*)pp - 1)) {
        copy_size = *((size_t*)pp - 1);
    }

    memcpy(new_ptr, pp, copy_size);

    free(pp);

    return new_ptr;
}

double sf_fragmentation() {
    // To be implemented.
    abort();
}

double sf_utilization() {
    // To be implemented.
    abort();
}
