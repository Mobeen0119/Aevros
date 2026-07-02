#include <stdint.h>
#include <stddef.h>
#include "buddy.h"
#include "slab.h"
#include "../../Include/screen.h"

#define SLAB_SLOTS_PER_CACHE 32
#define SLAB_NUM_CACHES 3

slab_t cache_32b;
slab_t cache_64b;
slab_t cache_128b;

static slab_t *const slab_caches[SLAB_NUM_CACHES] = {
    &cache_32b,
    &cache_64b,
    &cache_128b
};

void slab_init(slab_t *slab, int size)
{
    slab->bitmap = 0;
    slab->size = size;

    slab->first_slot = buddy_alloc(0);

    if(!slab->first_slot) {
        kprint("\nKERNEL PANICCCCCC\n");
    }
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

    if(index<0 || index>=SLAB_SLOTS_PER_CACHE) return;

    slab->bitmap &= ~(1 << index);
}

static uint32_t slab_used_count(void)
{
    uint32_t total = 0;
    for (int i = 0; i < SLAB_NUM_CACHES; i++)
        total += __builtin_popcount(slab_caches[i]->bitmap);
    return total;
}

uint32_t slab_objects_used(void)
{
    return slab_used_count();
}

uint32_t slab_objects_free(void)
{
    return (SLAB_NUM_CACHES * SLAB_SLOTS_PER_CACHE) - slab_used_count();
}

uint32_t slab_cache_used(slab_t *cache)
{
    return (uint32_t)__builtin_popcount(cache->bitmap);
}

uint32_t slab_cache_free(slab_t *cache)
{
    return SLAB_SLOTS_PER_CACHE - slab_cache_used(cache);
}

uint32_t slab_used_bytes(void)
{
    uint32_t total = 0;
    for (int i = 0; i < SLAB_NUM_CACHES; i++)
        total += slab_cache_used(slab_caches[i]) * slab_caches[i]->size;
    return total;
}

uint32_t slab_free_bytes(void)
{
    uint32_t total = 0;
    for (int i = 0; i < SLAB_NUM_CACHES; i++)
        total += slab_cache_free(slab_caches[i]) * slab_caches[i]->size;
    return total;
}