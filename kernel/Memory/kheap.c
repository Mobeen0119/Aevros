#include <stddef.h>
#include "slab.c"
#include "buddy.c"
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
void *kmalloc(size_t size)
{
    block_header_t *hdr;

    if (size <= 16)
    {
        hdr = (block_header_t *)slab_alloc(&cache_16b);
        hdr->type = SLAB;
        hdr->infor.cache = &cache_16b;
        return (void *)(hdr + 1);
    }

    if (size <= 32)
    {
        hdr = (block_header_t *)slab_alloc(&cache_32b);
        hdr->type = SLAB;
        hdr->infor.cache = &cache_32b;
        return (void *)(hdr + 1);
    }

    if (size <= 64)
    {
        hdr = (block_header_t *)slab_alloc(&cache_64b);
        hdr->type = SLAB;
        hdr->infor.cache = &cache_64b;
        return (void *)(hdr + 1);
    }

    int order = size_to_order(size);

    hdr = (block_header_t *)buddy_alloc(order);
    hdr->type = BUDDY;
    hdr->infor.order = order;

    return (void *)(hdr + 1);
}

void kfree(void *ptr)
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