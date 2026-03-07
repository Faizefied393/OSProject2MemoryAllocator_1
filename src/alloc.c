#include "alloc.h"

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define ALIGNMENT 16
#define MAGIC 0x12345678

static free_block *free_list = NULL;

/* Extra credit: keeps track of where next-fit should continue searching */
static free_block *next_fit_cursor = NULL;

/* Helper functions */

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
 * Ask the OS for a new chunk of memory.
 * We request enough space for the header plus the aligned payload.
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
 * Put a block back into the free list in memory-address order.
 * This helps later when we want to merge neighboring blocks.
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
 * Walk through the free list and merge blocks that are right next to each other.
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
 * Remove one specific block from the free list.
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
 * Split a block if it is big enough to leave behind a useful free block.
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

/* Required functions */

void *tumalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    size_t wanted = align_up(size);

    /* If the free list is empty, just get a new block from the OS */
    if (free_list == NULL) {
        free_block *new_block = request_from_os(wanted);
        if (new_block == NULL) {
            return NULL;
        }

        header *h = (header *)new_block;
        h->size = wanted;
        h->magic = MAGIC;

        return header_to_ptr(h);
    }

    /* If next-fit has no starting point yet, begin at the front */
    if (next_fit_cursor == NULL) {
        next_fit_cursor = free_list;
    }

    free_block *start = next_fit_cursor;
    free_block *curr = start;
    free_block *prev = NULL;

    /* Find the node before start so removal works correctly */
    if (curr != free_list) {
        prev = free_list;
        while (prev != NULL && prev->next != curr) {
            prev = prev->next;
        }
    }

    do {
        if (curr->size >= wanted) {
            split_block_if_possible(curr, wanted);

            /* If the block was split, curr->next now points to the leftover piece */
            free_block *remainder = curr->next;

            remove_block_from_free_list(curr, prev);

            /* Put the leftover piece back into the list if there is one */
            if (remainder != NULL) {
                if (prev == NULL) {
                    free_list = remainder;
                } else {
                    prev->next = remainder;
                }
                next_fit_cursor = remainder;
            } else {
                next_fit_cursor = free_list;
            }

            header *h = (header *)curr;
            h->size = wanted;
            h->magic = MAGIC;

            return header_to_ptr(h);
        }

        prev = curr;
        curr = curr->next;

        /* If we hit the end, wrap back around to the front */
        if (curr == NULL) {
            curr = free_list;
            prev = NULL;
        }

    } while (curr != NULL && curr != start);

    /* No free block was large enough, so request more memory */
    free_block *new_block = request_from_os(wanted);
    if (new_block == NULL) {
        return NULL;
    }

    header *h = (header *)new_block;
    h->size = wanted;
    h->magic = MAGIC;

    /* Reset cursor safely */
    next_fit_cursor = free_list;

    return header_to_ptr(h);
}

void *tucalloc(size_t num, size_t size) {
    if (num == 0 || size == 0) {
        return NULL;
    }

    /* Prevent overflow when multiplying */
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

    /* If the current block is already big enough, keep using it */
    if (old_header->size >= wanted) {
        return ptr;
    }

    /* Otherwise, get a new block and move the old data over */
    void *new_ptr = tumalloc(wanted);
    if (new_ptr == NULL) {
        return NULL;
    }

    memcpy(new_ptr, ptr, old_header->size);

    /* Zero out the extra space in the bigger block */
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

    /* Catch invalid frees or double frees */
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

    /*
     * Reset next-fit to a safe location after freeing/coalescing,
     * so it does not point to a block that may have been merged away.
     */
    next_fit_cursor = free_list;
}
