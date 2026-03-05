#ifndef CYB3053_PROJECT2_ALLOC_H
#define CYB3053_PROJECT2_ALLOC_H

#include <stddef.h>

/*
 * Metadata for allocated blocks.
 * This sits immediately before the user-visible pointer.
 */
typedef struct header {
    size_t size;   // size of user payload
    int magic;     // used to validate free/realloc
} header;

/*
 * Metadata for free blocks.
 * Free blocks form the free list.
 */
typedef struct free_block {
    size_t size;               // size of user payload
    struct free_block *next;   // next block in free list
} free_block;

void *tumalloc(size_t size);
void *tucalloc(size_t num, size_t size);
void *turealloc(void *ptr, size_t new_size);
void tufree(void *ptr);

#endif
