#include <stdint.h>
#include <stddef.h>
#include "buddy.h"
#include "slab.h"

slab_t cache_64b;
slab_t cache_32b;
slab_t cache_128b;

void slab_init(slab_t *slab, int size)
{
    slab->bitmap = 0;
    slab->size = size;

    slab->first_slot = buddy_alloc(0);
}

void slab_init_all()
{
    slab_init(&cache_64b, 64);
    slab_init(&cache_128b, 128);
    slab_init(&cache_32b, 32);
}

void *slab_alloc(slab_t *slab)
{

    uint32_t free_mask = ~slab->bitmap;
    if (free_mask == 0)
        return NULL;
    int free_bit = __builtin_ctz(free_mask);

    slab->bitmap |= (1 << free_bit);

    return (void *)((uint32_t)slab->first_slot + (free_bit * slab->size));
}

void slab_free(slab_t *slab, void *ptr)
{
    int index = ((uint32_t)ptr - (uint32_t)slab->first_slot) / slab->size;

    slab->bitmap &= ~(1 << index);
}

static uint32_t slab_used_count(void)
{
    uint32_t total = 0;

    slab_t *caches[3] = {&cache_32b, &cache_64b, &cache_128b};

    for (int i = 0; i < 3; i++)
    {
        uint32_t bm = caches[i]->bitmap;
        for (int k = 0; k < 32; k++)
        {
            if (bm & (1u << k))
                total++;
        }
    }
        return total;
    }

uint32_t slab_objects_used(void)
    {
        return slab_used_count();
    }
 
uint32_t slab_objects_free(void)
    {
        return (32 * 3) - slab_used_count();
    }

