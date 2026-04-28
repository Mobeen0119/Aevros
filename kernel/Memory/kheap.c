#include <stddef.h>
#include "slab.c"
#define MAX_ORDER 10

void *kmalloc(size_t size)
{
    if (size <= 16)
        return slab_alloc(&cache_16b);
    if (size <= 32)
        return slab_alloc(&cache_32b);
    if (size <= 64)
        return slab_alloc(&cache_64b);

    int order = size_to_order(size);
    return buddy_alloc(order);
}