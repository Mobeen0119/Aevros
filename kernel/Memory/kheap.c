#include "kheap.h"
#include <stddef.h>

#include "buddy.h"
#include "slab.h"

#define MAX_ORDER 10

#define SLAB_NUM_CACHES 3
extern slab_t cache_32b;
extern slab_t cache_64b;
extern slab_t cache_128b;
static slab_t *const slab_caches[SLAB_NUM_CACHES] = {
    &cache_32b,
    &cache_64b,
    &cache_128b};

int size_to_order(size_t size)
{
    int order = 0;
    size_t block = 4096;

    while (block < size)
    {
        block *= 2;
        order++;
    }
    return order;
}

void *kmalloc_raw(size_t size)
{
    if (size <= 32)
        return slab_alloc(&cache_32b);

    if (size <= 64)
        return slab_alloc(&cache_64b);

    if (size <= 128)
        return slab_alloc(&cache_128b);

    size_t total = size + sizeof(block_header_t);

    int order = size_to_order(total);
    block_header_t *hdr = (block_header_t *)buddy_alloc(order);
    if (!hdr)
        return NULL;

    hdr->type = BUDDY;
    hdr->infor.order = order;

    return (void *)(hdr + 1);
}

void kfree_raw(void *ptr)
{
    if (!ptr)
        return;

    for (int i = 0; i < SLAB_NUM_CACHES; i++)
    {
        slab_t *cache = slab_caches[i];
        uint32_t start = (uint32_t)cache->first_slot;
        uint32_t end = start + 4096;
        uint32_t p = (uint32_t)ptr;

        if (p >= start && p < end)
        {
            slab_free(cache, ptr);
            return;
        }
    }

    block_header_t *hdr = ((block_header_t *)ptr) - 1;
    if (hdr->type == BUDDY)
    {
        buddy_free(hdr, hdr->infor.order);
    }
}

uint32_t heap_used_bytes(void)
{
    return slab_used_bytes() + buddy_used_memory();
}

uint32_t heap_free_bytes(void)
{
    return slab_free_bytes() + buddy_free_memory();
}

uint32_t heap_total_bytes(void)
{
    return heap_used_bytes() + heap_free_bytes();
}

uint32_t heap_largest_free_block(void)
{
    return buddy_largest_free_block();
}

uint32_t heap_live_allocation(void)
{
    return slab_objects_used() + buddy_live_allocations();
}
