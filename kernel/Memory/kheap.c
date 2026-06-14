#include "kheap.h"
#include <stddef.h>

#include "buddy.h"
#include "slab.h"

#define MAX_ORDER 10

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
    block_header_t *hdr;

    if (size <= 32)
    {
        hdr = (block_header_t *)slab_alloc(&cache_32b);
        if (!hdr)
            return NULL;
        hdr->type = SLAB;
        hdr->infor.cache = &cache_32b;
        return (void *)(hdr + 1);
    }

    if (size <= 64)
    {
        hdr = (block_header_t *)slab_alloc(&cache_64b);
        if (!hdr)
            return NULL;
        hdr->type = SLAB;
        hdr->infor.cache = &cache_64b;
        return (void *)(hdr + 1);
    }

    if (size <= 128)
    {
        hdr = (block_header_t *)slab_alloc(&cache_128b);
        if (!hdr)
            return NULL;
        hdr->type = SLAB;
        hdr->infor.cache = &cache_128b;
        return (void *)(hdr + 1);
    }

    int order = size_to_order(size);

    hdr = (block_header_t *)buddy_alloc(order);
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

    block_header_t *hdr = ((block_header_t *)ptr) - 1;

    if (hdr->type == SLAB)
    {
        slab_free(hdr->infor.cache, hdr);
    }
    else if (hdr->type == BUDDY)
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
