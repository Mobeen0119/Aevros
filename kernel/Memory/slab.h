#include <stdint.h>
#include <stddef.h>
#include "buddy.h"

typedef struct slab_cache
{
    uint32_t bitmap;
    void *first_slot;
    int size;
} slab_t;


extern slab_t cache_32b;
extern slab_t cache_64b;
extern slab_t cache_128b;


void slab_init(slab_t* slab, int size);

void slab_init_all();

void* slab_alloc(slab_t* slab);

void slab_free(slab_t* slab, void* ptr);

static uint32_t slab_used_count();

uint32_t slab_objects_used(void);
uint32_t slab_objects_free(void);