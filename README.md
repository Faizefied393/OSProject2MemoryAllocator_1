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
OSProject2MemoryAllocator_1/
├── src/
│   ├── alloc.h      # Public allocator API + metadata structs
│   ├── alloc.c      # Allocator implementation
│   └── main.c       # Simple test program
├── build/           # Build output (generated)
├── CMakeLists.txt   # Build configuration
└── README.md        # Project documentation
