#include "alloc.h"

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define ALIGNMENT 16
#define MAGIC 0x12345678

static free_block *free_list = NULL;

/* Helper Functions */

static size_t align_up(size_t size) {
    return (size + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);
}

static header *ptr_to_header(void *ptr) {
    return ((header *)ptr) - 1;
}

static void *header_to_ptr(header *h) {
    return (void *)(h + 1);
}

/*
 * Request a new block from the OS.
 * Total bytes requested = metadata + aligned payload.
 */
static free_block *request_from_os(size_t aligned_payload_size) {
    size_t total_size = align_up(sizeof(header) + aligned_payload_size);

    void *mem = sbrk((intptr_t)total_size);
    if (mem == (void *)-1) {
        return NULL;
    }

    free_block *block = (free_block *)mem;
    block->size = total_size - sizeof(header);
    block->next = NULL;
    return block;
}

/*
 * Insert a free block into the free list in address order.
 * This makes coalescing easy and reliable.
 */
static void insert_free_block_sorted(free_block *block) {
    if (block == NULL) {
        return;
    }

    if (free_list == NULL || block < free_list) {
        block->next = free_list;
        free_list = block;
        return;
    }

    free_block *curr = free_list;
    while (curr->next != NULL && curr->next < block) {
        curr = curr->next;
    }

    block->next = curr->next;
    curr->next = block;
}

/*
 * Merge adjacent free blocks.
 */
static void coalesce_free_list(void) {
    free_block *curr = free_list;

    while (curr != NULL && curr->next != NULL) {
        char *curr_end = (char *)curr + sizeof(header) + curr->size;

        if (curr_end == (char *)curr->next) {
            curr->size += sizeof(header) + curr->next->size;
            curr->next = curr->next->next;
        } else {
            curr = curr->next;
        }
    }
}

/*
 * Remove a specific block from the free list.
 */
static void remove_block_from_free_list(free_block *block, free_block *prev) {
    if (prev == NULL) {
        free_list = block->next;
    } else {
        prev->next = block->next;
    }
    block->next = NULL;
}

/*
 * Split a block if there is enough space left to form another valid free block.
 */
static void split_block_if_possible(free_block *block, size_t wanted_size) {
    size_t min_remainder = sizeof(header) + ALIGNMENT;

    if (block->size < wanted_size + min_remainder) {
        return;
    }

    char *remainder_addr = (char *)block + sizeof(header) + wanted_size;
    free_block *remainder = (free_block *)remainder_addr;

    remainder->size = block->size - wanted_size - sizeof(header);
    remainder->next = block->next;

    block->size = wanted_size;
    block->next = remainder;
}

/* Required Functions */

void *tumalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    size_t wanted = align_up(size);

    /* First Fit search */
    free_block *curr = free_list;
    free_block *prev = NULL;

    while (curr != NULL) {
        if (curr->size >= wanted) {
            split_block_if_possible(curr, wanted);

            /* If split happened, curr->next is the remainder block. */
            free_block *remainder = curr->next;

            remove_block_from_free_list(curr, prev);

            /* Restore remainder into list if split happened. */
            if (remainder != NULL) {
                if (prev == NULL) {
                    free_list = remainder;
                } else {
                    prev->next = remainder;
                }
            }

            header *h = (header *)curr;
            h->size = wanted;
            h->magic = MAGIC;

            return header_to_ptr(h);
        }

        prev = curr;
        curr = curr->next;
    }

    /* No fit found, ask OS for more heap */
    free_block *new_block = request_from_os(wanted);
    if (new_block == NULL) {
        return NULL;
    }

    header *h = (header *)new_block;
    h->size = wanted;
    h->magic = MAGIC;

    return header_to_ptr(h);
}

void *tucalloc(size_t num, size_t size) {
    if (num == 0 || size == 0) {
        return NULL;
    }

    /* overflow protection */
    if (num > SIZE_MAX / size) {
        return NULL;
    }

    size_t total = num * size;
    void *ptr = tumalloc(total);
    if (ptr == NULL) {
        return NULL;
    }

    memset(ptr, 0, total);
    return ptr;
}

void *turealloc(void *ptr, size_t new_size) {
    if (ptr == NULL) {
        return tumalloc(new_size);
    }

    if (new_size == 0) {
        tufree(ptr);
        return NULL;
    }

    header *old_header = ptr_to_header(ptr);

    if (old_header->magic != MAGIC) {
        return NULL;
    }

    size_t wanted = align_up(new_size);

    /* If current block is already big enough, keep it */
    if (old_header->size >= wanted) {
        return ptr;
    }

    /* Otherwise allocate a new block */
    void *new_ptr = tumalloc(wanted);
    if (new_ptr == NULL) {
        return NULL;
    }

    memcpy(new_ptr, ptr, old_header->size);

    /* Optional but nice: zero the newly added area */
    if (wanted > old_header->size) {
        memset((char *)new_ptr + old_header->size, 0, wanted - old_header->size);
    }

    tufree(ptr);
    return new_ptr;
}

void tufree(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    header *h = ptr_to_header(ptr);

    /* detect invalid free / double free */
    if (h->magic != MAGIC) {
        fprintf(stderr, "tufree: invalid free or double free detected\n");
        return;
    }

    h->magic = 0;

    free_block *block = (free_block *)h;
    block->size = h->size;
    block->next = NULL;

    insert_free_block_sorted(block);
    coalesce_free_list();
}
