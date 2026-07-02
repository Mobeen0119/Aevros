#ifndef BUDDY_H
#define BUDDY_H

#include <stddef.h>

#define MAX_ORDER 16

typedef enum
{
    SLAB = 1,
    BUDDY = 2
} alloc_type_t;

struct slab_cache;
typedef struct block_header
{
    uint32_t size;
    alloc_type_t type;

    union
    {
        struct slab_cache *cache;
        uint32_t order;

    } infor;
} block_header_t;


typedef struct _buddy_block
{
    struct _buddy_block *next; // next block in the free list
} buddy_block_t;


void buddy_init(uint32_t start, uint32_t end);

void *buddy_alloc(int order);

void add_to_list(void *ptr, int order);

void buddy_debug_list(int order);

void remove_from_list(void *ptr, int order);

void buddy_free(void *ptr, int order);

uint32_t buddy_total_memory(void);
uint32_t buddy_free_memory(void);
uint32_t buddy_used_memory(void);
uint32_t buddy_fragmentation(void);

uint32_t buddy_largest_free_block(void);
uint32_t buddy_free_count_at_order(int order);
uint32_t buddy_live_allocations(void);


#endif 
