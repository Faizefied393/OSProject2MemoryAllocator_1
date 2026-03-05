# OSProject2MemoryAllocator_1

A custom dynamic memory allocator written in C for an Operating Systems project.  
This project implements replacement versions of standard memory-management functions using a free list, block splitting, coalescing, and heap expansion through `sbrk()`.

## Overview

This allocator provides custom implementations of:

- `tumalloc(size_t size)`
- `tucalloc(size_t num, size_t size)`
- `turealloc(void *ptr, size_t new_size)`
- `tufree(void *ptr)`

The allocator manages memory manually by:

- aligning allocations to **16 bytes**
- storing metadata in a block header
- maintaining a **free list**
- reusing freed blocks
- splitting large free blocks when possible
- coalescing adjacent free blocks to reduce fragmentation
- requesting more heap memory from the OS using `sbrk()`

## Features

- **16-byte alignment** for returned user pointers
- **First-fit allocation strategy**
- **Block splitting** when a free block is larger than needed
- **Address-ordered free list insertion**
- **Coalescing of neighboring free blocks**
- **Overflow protection** in `tucalloc`
- **Basic invalid free / double free detection** using a magic value
- Cleanly passes **Valgrind** with no leaks or memory errors in the included test

## Project Structure
```text
OSProject2MemoryAllocator_1/
- src/
  - alloc.h      # Function declarations and block structure definitions
  - alloc.c      # Main allocator implementation
  - main.c       # Small test program for checking allocator behavior
- build/         # Compiled output generated during the build
- CMakeLists.txt # CMake configuration file
- README.md      # Project documentation
```

## Memory Layout

Each allocated block stores metadata immediately before the user-visible pointer.

### Allocated block layout

```text
 header: size, magic: user data returned to the program 
```

### Free block layout

```text
 free_block: size, next: unused memory available for reuse 
```

## Function Descriptions

### `tumalloc(size_t size)`

Allocates a block of memory for the caller.

Behavior:
- returns `NULL` if `size == 0`
- aligns requested size to 16 bytes
- searches the free list using **first fit**
- splits a free block if enough space remains
- removes the chosen block from the free list
- requests more memory from the OS with `sbrk()` if no block fits

### `tucalloc(size_t num, size_t size)`

Allocates space for an array and initializes it to zero.

Behavior:
- returns `NULL` if `num == 0` or `size == 0`
- checks for integer overflow before multiplying
- calls `tumalloc(num * size)`
- clears memory using `memset`

### `turealloc(void *ptr, size_t new_size)`

Resizes a previously allocated block.

Behavior:
- if `ptr == NULL`, behaves like `tumalloc`
- if `new_size == 0`, behaves like `tufree` and returns `NULL`
- validates the block using the magic value
- returns the same pointer if the current block is already large enough
- otherwise allocates a new block, copies old data, frees the old block, and returns the new pointer

### `tufree(void *ptr)`

Frees a previously allocated block.

Behavior:
- does nothing if `ptr == NULL`
- validates the block using the magic value
- detects invalid free / double free attempts
- inserts the block back into the free list in address order
- coalesces adjacent free blocks

## Build Instructions

### Using CMake

```bash
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

### Run the program

```bash
./cyb3053_project2
```

## Valgrind Testing

To verify correctness and memory safety:

```bash
valgrind --leak-check=full --show-leak-kinds=all ./cyb3053_project2
```

Expected result:
- **0 memory errors**
- **0 bytes in use at exit**
- **no leaks are possible**

## Example Output

The included test driver in `main.c` performs:
- a standard allocation with `tumalloc`
- a zero-initialized allocation with `tucalloc`
- a resize with `turealloc`
- cleanup with `tufree`

Example output may include printed integer values from test arrays.

## Allocation Strategy

This implementation currently uses:

- **First Fit**: the allocator scans the free list from the beginning and uses the first block large enough to satisfy the request.

## Extra Credit

This allocator can be extended to support a **Next Fit** strategy by keeping a cursor to the last search position and continuing future searches from there instead of restarting at the head of the free list.

A next-fit version would:
- reduce repeated scanning from the front of the list
- potentially improve performance in some allocation patterns
- reuse the existing split, free, and coalescing logic

## Design Notes

Key implementation choices:

- **Header-based metadata** keeps block size and validity information close to the payload
- **Address-sorted free list** makes coalescing simpler and more reliable
- **Coalescing** reduces fragmentation by merging adjacent free blocks
- **Magic field validation** helps catch invalid frees and double frees during testing

## Limitations

- Uses `sbrk()`, which is low-level and mainly for educational purposes
- Not thread-safe
- Does not shrink the program break after freeing memory
- Does not implement advanced allocation policies such as best fit or segregated free lists

## Educational Purpose

This project is intended as a learning exercise in:

- low-level memory management
- heap allocation design
- fragmentation control
- pointer arithmetic
- custom allocator implementation in C

## Author

Faiz Tariq

## License

This project is for educational/coursework use unless otherwise specified.

