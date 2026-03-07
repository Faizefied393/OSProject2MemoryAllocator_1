#ifndef CYB3053_PROJECT2_ALLOC_H
#define CYB3053_PROJECT2_ALLOC_H

#include <stddef.h>

/*
 * Header used for blocks that have been allocated.
 * It is stored right before the pointer returned to the user.
 */
typedef struct header {
    size_t size;   // size of the usable memory block
    int magic;     // helps check for invalid free or realloc calls
} header;

/*
 * Header used for blocks that are currently free.
 * These blocks are linked together in the free list.
 */
typedef struct free_block {
    size_t size;               // size of the usable memory block
    struct free_block *next;   // points to the next free block
} free_block;

void *tumalloc(size_t size);
void *tucalloc(size_t num, size_t size);
void *turealloc(void *ptr, size_t new_size);
void tufree(void *ptr);

#endif
