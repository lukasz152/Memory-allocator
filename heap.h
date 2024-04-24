//
// Created by Konto on 22.11.2023.
//
#include <stdint.h>
#include <stdlib.h>
#ifndef NEW_HEAP_H
#define NEW_HEAP_H
struct memory_manager_t
{
    void *memory_start;
    size_t memory_size;
    struct memory_chunk_t *first_memory_chunk;
};

struct memory_chunk_t
{
    struct memory_chunk_t* prev;
    struct memory_chunk_t* next;
    size_t size;
    int free;
    int du;
    int real_memory;
    int validate_sum;
};
void *heap_malloc(size_t size);
struct memory_manager_t memory_manager;
int heap_setup(void);
int heap_validate(void);
void heap_clean(void);
void* heap_calloc(size_t number, size_t size);
void* heap_realloc(void* memblock, size_t count);
void  heap_free(void* memblock);
size_t   heap_get_largest_used_block_size(void);

enum pointer_type_t
{
    pointer_null,
    pointer_heap_corrupted,
    pointer_control_block,
    pointer_inside_fences,
    pointer_inside_data_block,
    pointer_unallocated,
    pointer_valid
};
enum pointer_type_t get_pointer_type(const void* const pointer);
#endif //NEW_HEAP_H
