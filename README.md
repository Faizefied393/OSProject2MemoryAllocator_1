# OSProject2MemoryAllocator_1

A custom dynamic memory allocator written in C for an Operating Systems project.  
This project replaces the standard `malloc/calloc/realloc/free` family with your own allocator built on a free list, block splitting/coalescing, and heap growth using `sbrk()`.

## What This Project Does

This allocator provides custom implementations of:

- `tumalloc(size_t size)`
- `tucalloc(size_t num, size_t size)`
- `turealloc(void *ptr, size_t new_size)`
- `tufree(void *ptr)`

Under the hood, it manages heap memory manually by:

- Aligning allocations to **16 bytes**
- Storing metadata in a header right before the user payload
- Tracking available blocks in a **free list**
- Reusing freed blocks when possible
- Splitting oversized blocks to avoid wasting space
- Coalescing adjacent free blocks to reduce fragmentation
- Expanding the heap with `sbrk()` when no free block can satisfy a request

## Key Features

- **16-byte aligned** pointers returned to the caller  
- **First-fit** search strategy over the free list  
- **Splits** large free blocks when there’s enough leftover space  
- Inserts freed blocks **in address order** (makes merging easier)  
- **Coalesces** neighboring free blocks to reduce fragmentation  
- Includes **overflow protection** in `tucalloc`  
- Basic **invalid free / double free detection** using a magic value  
- Passes **Valgrind** in the provided test with **no leaks** and **no memory errors**

## Project Layout

```text id="f4w4ol"
```
OSProject2MemoryAllocator_1/
├── src/
│   ├── alloc.h      # Public allocator API + metadata structs
│   ├── alloc.c      # Allocator implementation
│   └── main.c       # Simple test program
├── build/           # Build output (generated)
├── CMakeLists.txt   # Build configuration
└── README.md        # Project documentation
Memory Layout
```
```
Each block has metadata stored immediately before the pointer returned to the user.
'''
Allocated Block
+-------------------+----------------------+
| header            | user payload         |
| size, magic       | returned to caller   |
+-------------------+----------------------+
Free Block
+-------------------+----------------------+
| free_block        | unused payload       |
| size, next        | available space      |
+-------------------+----------------------+
How Each Function Works
tumalloc(size_t size)

Allocates memory for the caller.

Behavior:

Returns NULL if size == 0

Rounds the request up to a 16-byte boundary

Searches the free list using first fit

Splits a block if the leftover space can form another valid free block

If nothing fits, requests more heap space using sbrk()

tucalloc(size_t num, size_t size)

Allocates an array and zero-initializes it.

Behavior:

Returns NULL if num == 0 or size == 0

Checks for multiplication overflow (num * size)

Uses tumalloc for the allocation

Clears memory with memset

turealloc(void *ptr, size_t new_size)

Resizes an existing allocation.

Behavior:

If ptr == NULL, it acts like tumalloc(new_size)

If new_size == 0, it frees the block and returns NULL

Validates the block using the magic field

Reuses the same block if it already has enough space

Otherwise allocates a new block, copies data, frees the old block, and returns the new pointer

tufree(void *ptr)

Frees a previously allocated block.

Behavior:

Does nothing if ptr == NULL

Validates the block using the magic field

Catches basic invalid free / double free attempts

Inserts the block back into the free list (sorted by address)

Merges adjacent free blocks when possible

Build and Run
Build (CMake)
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
Run
./cyb3053_project2
Valgrind Check
valgrind --leak-check=full --show-leak-kinds=all ./cyb3053_project2

Expected:

0 errors

0 bytes in use at exit

no leaks possible

Allocation Strategy

This allocator uses First Fit: it scans the free list from the front and takes the first block that can satisfy the request.

Extra Credit Idea: Next Fit

A simple extension is Next Fit, which keeps a cursor to where the last search ended and continues from there next time.

Potential benefits:

Less repeated scanning from the start of the list

Faster allocation in some workloads

Can reuse the same split/free/coalesce logic

Design Notes

Metadata headers keep block info right next to the payload

An address-sorted free list makes coalescing straightforward

Coalescing helps fight fragmentation over time

Magic-value validation is a helpful debugging tool during testing

Limitations

Uses sbrk() (fine for learning, not modern production allocators)

Not thread-safe

Does not return memory back to the OS once allocated

Doesn’t implement advanced strategies (best fit, bins, segregated lists, etc.)

Author

Faiz Tariq
```
