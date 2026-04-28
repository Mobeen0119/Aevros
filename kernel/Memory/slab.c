#include <stdint.h>

typedef struct
{
    uint32_t bitmap;
    void *first_slot;
} slab_t;

slab_t cache_16b;
slab_t cache_32b;
slab_t cache_64b;