//
// Created by Konto on 22.11.2023.
//
#include "heap.h"
#include <stdio.h>
#include "tested_declarations.h"
#include "rdebug.h"
#include <string.h>
void add_16_plus_16_fences(struct memory_chunk_t **current_chunk, size_t user_size) {
    for (int i = 0; i < 16; i++) {
        *((char *) (*current_chunk) + sizeof(struct memory_chunk_t) + i) = '#';
        *((char *) (*current_chunk) + sizeof(struct memory_chunk_t) + 16 + user_size + i) = '#';
    }
}
void return_to_start_of_list(struct memory_chunk_t **current_chunk) {
    for (; (*current_chunk)->prev != NULL;) {
        (*current_chunk) = (*current_chunk)->prev;
    }
}
void return_to_end_of_list(struct memory_chunk_t **current_chunk) {
    for (; (*current_chunk)->next != NULL;) {
        (*current_chunk) = (*current_chunk)->next;
    }
}
int calculate_byte_value_sum_of_chunk(struct memory_chunk_t *current_chunk) {
    int val = 0;
    for (size_t i = 0; i < sizeof(struct memory_chunk_t) - sizeof(int); i++) {
        val += *((char *) current_chunk + i);
    }
    current_chunk->validate_sum = val;
    return val;
}
int check_validate_value(struct memory_chunk_t *current_chunk) {
    int val = 0;
    for (size_t i = 0; i < sizeof(struct memory_chunk_t) - sizeof(int); i++) {
        val += *((char *) (current_chunk) + i);
    }
    if (val != current_chunk->validate_sum) {
        return 1;
    }
    return 0;
}
struct memory_chunk_t *connecting_free_blocks(struct memory_chunk_t *chunk) {
    struct memory_chunk_t *current_chunk = chunk;
    while (current_chunk != NULL && current_chunk->next != NULL) {
        // Sprawdź następny blok
        if (current_chunk->next != NULL && current_chunk->next->free == 1 && current_chunk->free == 1) {
            struct memory_chunk_t *new = current_chunk;
            //char *ostati_adres = (char *) (current_chunk->next->next);
            new->size = (char *) (current_chunk->next) - (char *) (current_chunk) - sizeof(struct memory_chunk_t) - 32+ (current_chunk->next->real_memory)+32+sizeof(struct memory_chunk_t);
            new->free = 1;
            new->real_memory=new->size;
            if (current_chunk->prev != NULL) {
                new->validate_sum= calculate_byte_value_sum_of_chunk(current_chunk);
                new->prev = current_chunk->prev;
                current_chunk->prev->next = new;
                current_chunk->prev->validate_sum= calculate_byte_value_sum_of_chunk(current_chunk->prev);
            } else {
                new->validate_sum= calculate_byte_value_sum_of_chunk(current_chunk);
                new->prev = NULL;
            }
            struct memory_chunk_t *next_ch = current_chunk->next;
            if (next_ch->next != NULL) {
                new->next = next_ch->next;
                next_ch->next->prev = new;
                next_ch->next->validate_sum= calculate_byte_value_sum_of_chunk(next_ch->next);
                new->validate_sum= calculate_byte_value_sum_of_chunk(new);
            } else {
                new->validate_sum= calculate_byte_value_sum_of_chunk(new);
                new->next = NULL;
            }
            continue;
        }
        if (current_chunk->next == NULL) { break; }
        current_chunk = current_chunk->next;
    }
    return_to_start_of_list(&current_chunk);
    return current_chunk;
}
void *heap_malloc(size_t size) {
    if (memory_manager.memory_start == NULL || heap_validate() == 1 || size == 0) return NULL;
    if (memory_manager.first_memory_chunk == NULL) {
        void *ptr = custom_sbrk((size + sizeof(struct memory_chunk_t) + 32));
        if (ptr == (void *) -1) return NULL;
        memory_manager.memory_start = ptr;
        struct memory_chunk_t *chunk = (struct memory_chunk_t *) ((char *) (memory_manager.memory_start));
        add_16_plus_16_fences(&chunk, size);
        chunk->free = 0;
        chunk->size = size;
        chunk->next = NULL;
        chunk->prev = NULL;
        chunk->real_memory=size;
        chunk->validate_sum = calculate_byte_value_sum_of_chunk(chunk);
        memory_manager.first_memory_chunk = chunk;
        memory_manager.memory_size += sizeof(struct memory_chunk_t) + 32 + size;
        return (char *) (chunk) + sizeof(struct memory_chunk_t) + 16;
    } else {
        struct memory_chunk_t *chunk_next = memory_manager.first_memory_chunk;
        //sprawdzenie czy w zwolnionym bloku zmiesci sie nowy blok
        for (; chunk_next->next != NULL;) {
            if (chunk_next->free == 1 && chunk_next->size >= size) {     //150
                chunk_next->free = 0;
                chunk_next->size=size;
                add_16_plus_16_fences(&chunk_next,size);
                chunk_next->validate_sum = calculate_byte_value_sum_of_chunk(chunk_next);
                return (char *) (chunk_next) + sizeof(struct memory_chunk_t) + 16;
            }
            chunk_next = chunk_next->next;
        }
        //dodanie nowego bloku
        struct memory_chunk_t *new_block = custom_sbrk((size + sizeof(struct memory_chunk_t) + 32));
        if (new_block == (void *) -1) return NULL;
        memory_manager.memory_size += size + 32 + sizeof(struct memory_chunk_t);
        add_16_plus_16_fences(&new_block, size);
        new_block->free = 0;
        new_block->size = size;
        new_block->prev = chunk_next;
        new_block->next = NULL;
        new_block->real_memory=size;
        new_block->validate_sum = calculate_byte_value_sum_of_chunk(new_block);
        chunk_next->next = new_block;
        chunk_next->validate_sum = calculate_byte_value_sum_of_chunk(chunk_next);
        return (char *) (new_block) + sizeof(struct memory_chunk_t) + 16;
    }
}
void heap_free(void *memblock) {
    if (memblock == NULL || memory_manager.memory_start == NULL || memory_manager.memory_size == 0 ||
        heap_validate() == 1 || get_pointer_type(memblock) != pointer_valid)
        return;
    struct memory_chunk_t *current = memory_manager.first_memory_chunk;
    while (1) {
        if ((char *) (current) + sizeof(struct memory_chunk_t) + 16 == (char *) memblock) {
            current->free = 1;
            if(current->next!=NULL)
                current->size =(char *) (current->next) - (char *) (current) - sizeof(struct memory_chunk_t) - 32;
            current->validate_sum= calculate_byte_value_sum_of_chunk(current);
            //connecting free blocks;
            current = connecting_free_blocks(memory_manager.first_memory_chunk);
            return_to_end_of_list(&current);
            if (current->next == NULL && current->free == 1) {
                if (current->prev != NULL) {
                    current = current->prev;//??
                    memory_manager.memory_size-=(current->next->real_memory+sizeof(struct memory_chunk_t)+32);
                    custom_sbrk((current->next->real_memory+sizeof(struct memory_chunk_t)+32)* (-1));
                    current->next=NULL;
                    current->validate_sum= calculate_byte_value_sum_of_chunk(current);
                    return;
                } else {
                    size_t ala=sizeof(struct memory_chunk_t) + current->real_memory + 32;
                    memory_manager.memory_size-=(sizeof(struct memory_chunk_t) + current->real_memory + 32);
                    memory_manager.memory_start = custom_sbrk((-1) * (ala));
                    memory_manager.first_memory_chunk = NULL;
                    return;
                }
            }
            break;
        }
        if (current->next == NULL) {
            break;
        }
        current = current->next;
    }
}
int heap_validate(void) {
    if (memory_manager.memory_start == NULL && memory_manager.first_memory_chunk == NULL) {
        return 2;
    }
    struct memory_chunk_t *chunk = memory_manager.first_memory_chunk;
    if (chunk == NULL) {
        return 0;
    }
    while (1) {
        if (check_validate_value(chunk)) { return 3; }
        for (int j = 0; j < 16; j++) {
            if (*((char *) (chunk) + sizeof(struct memory_chunk_t) + j) != '#') { return 1; }
            if (*((char *) (chunk) + sizeof(struct memory_chunk_t) + chunk->size + 16 + j) != '#') { return 1; }
        }
        if (chunk->next == NULL) break;
        chunk = chunk->next;
    }
    return 0;
}
int heap_setup(void) {
    void *ptr = custom_sbrk(0);
    if (ptr == (void *) -1) return -1;
    memory_manager.memory_start = ptr;
    memory_manager.memory_size = 0;
    memory_manager.first_memory_chunk = NULL;
    return 0;
}
void heap_clean(void) {
    custom_sbrk(memory_manager.memory_size * (-1));
    memory_manager.memory_size = 0;
    memory_manager.first_memory_chunk = NULL;
    memory_manager.memory_start = NULL;
}
void* heap_calloc(size_t number, size_t size)
{
    if(number*size == 0) {
        return NULL;
    }
    char *tmp = heap_malloc(number*size);
    if(!tmp) {
        return NULL;
    }
    memset(tmp, 0, size*number);

    return tmp;
}

void *heap_realloc(void *memblock, size_t count) {
    struct memory_chunk_t* current=memory_manager.first_memory_chunk;
    if(memblock==NULL && count ==0) return NULL;
    if(memblock==NULL){
        memblock=heap_malloc(count);
        return (char*)(memblock);
    }
    if(heap_validate()!=0 || get_pointer_type(memblock)!=pointer_valid) return NULL;
    if (count == 0 && (char*)(memblock)!=NULL&&(char *) (current) + sizeof(struct memory_chunk_t) + 16 == (char *) memblock) {
        current->size = current->real_memory;
        current->free = 1;
        current->validate_sum = calculate_byte_value_sum_of_chunk(current);
        heap_free(current);
        return NULL;
    }
    while(1){
        if ((char *) (current) + sizeof(struct memory_chunk_t) + 16 == (char *) memblock) {
            if(count==current->size) return (char *) (current) + sizeof(struct memory_chunk_t) + 16;
            if(count<(size_t)current->real_memory){
                current->size=count;
                add_16_plus_16_fences(&current,current->size);
                current->validate_sum= calculate_byte_value_sum_of_chunk(current);
                return (char *) (current) + sizeof(struct memory_chunk_t) + 16;
            }
            if(current->next!=NULL&&current->next->free==1&& (current->next->real_memory+current->real_memory+sizeof(struct memory_chunk_t)+32)>=(unsigned int)count){
                current->size=count;
                current->real_memory=current->next->real_memory+current->real_memory+sizeof(struct memory_chunk_t)+32;
                current->free=0;
                if(current->next->next!=NULL&&current->next->next->prev!=NULL){
                    current->next->next->prev=current;
                    current->next=current->next->next;
                }
                else {
                    current->next=NULL;
                }
                add_16_plus_16_fences(&current,count);
                current->next->validate_sum= calculate_byte_value_sum_of_chunk(current->next);
                current->validate_sum= calculate_byte_value_sum_of_chunk(current);
                return (char*)(current)+sizeof(struct memory_chunk_t)+16;
            }
            if(current->next==NULL){
                memory_manager.memory_start = custom_sbrk(count - current->size);
                if (memory_manager.memory_start == (void *) -1)
                    return NULL;
                memory_manager.memory_size+=count-current->size;
                current->size=count;
                current->real_memory=count;
                add_16_plus_16_fences(&current,current->size);
                current->validate_sum= calculate_byte_value_sum_of_chunk(current);
                return (char*)(current)+sizeof(struct memory_chunk_t)+16;
            }
            struct memory_chunk_t *new=(struct memory_chunk_t*)((char*)heap_malloc(count)-sizeof(struct memory_chunk_t)-16);
            if((char*)new+sizeof(struct memory_chunk_t)+16==NULL) return NULL;
            for(int i=0;i<(int)current->size;i++){
                *((char*)(new)+sizeof(struct memory_chunk_t)+16+i)=*((char*)(current)+sizeof(struct memory_chunk_t)+16+i);//przekopiowanie danych
            }
            current->free=1;
            current->size=current->real_memory;
            current->validate_sum= calculate_byte_value_sum_of_chunk(current);
            return (char*)(new)+sizeof(struct memory_chunk_t)+16;
        }
        if(current->next==NULL) break;
        current=current->next;
    }
    return NULL;
}

size_t heap_get_largest_used_block_size(void) {
    if(memory_manager.first_memory_chunk==NULL ||heap_validate()) return 0;
    struct memory_chunk_t *chunk=memory_manager.first_memory_chunk;
    size_t size_of_the_largest_block=0;
    for(int i=0;chunk!=NULL;i++){
        if(size_of_the_largest_block<chunk->size && chunk->free!=1){
            size_of_the_largest_block=chunk->size;
        }
        if(chunk->next==NULL) break;
        chunk=chunk->next;
    }
    return size_of_the_largest_block;
}

enum pointer_type_t get_pointer_type(const void *const pointer) {
    if (pointer == NULL) return pointer_null;
    struct memory_chunk_t *chunk = memory_manager.first_memory_chunk;
    if(chunk==NULL) return pointer_unallocated;
    while (1) {
        if(chunk->free==1&& (char*)(chunk)+sizeof(struct memory_chunk_t)<=(char*)(pointer) && (char*)(chunk)+sizeof(struct memory_chunk_t)+16+chunk->size+16>=(char*)(pointer)){ //zmieniles znaki jak cos < >
            return pointer_unallocated;
        }
        if(chunk->free==0) {
            for (int j = 0; j < 16; j++) {
                if (*((char *) (chunk) + sizeof(struct memory_chunk_t) + j) != '#') { return pointer_heap_corrupted; }
                if (*((char *) (chunk) + sizeof(struct memory_chunk_t) + 16 + chunk->size + j) !='#') { return pointer_heap_corrupted; }
                if ((char *) pointer ==(char *) (chunk) + sizeof(struct memory_chunk_t) + j) { return pointer_inside_fences; }
                if ((char *) pointer == (char *) (chunk) + sizeof(struct memory_chunk_t) + 16 + chunk->size +j)
                { return pointer_inside_fences; }
            }
        }
        if ((char *) chunk <= (char *) pointer && (char *) chunk + sizeof(struct memory_chunk_t) > (char *) pointer &&chunk->free == 0) {
            return pointer_control_block;
        }
        for (size_t j = 1; j < chunk->size; j++) {
            if ((char *) (chunk) + sizeof(struct memory_chunk_t) + 16 + j ==
                (char *) (pointer)) { return pointer_inside_data_block; }
        }
        if ((char *) chunk + sizeof(struct memory_chunk_t) + 16 == (char *) (pointer)&& chunk->free==0) return pointer_valid;
        if (chunk->next == NULL)break;
        chunk = chunk->next;
    }
    return pointer_unallocated;
}
int main(void) {
    return 0;
}
