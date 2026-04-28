#include <stdint.h>

typedef struct
{
    uint32_t bitmap;
    void *first_slot;
    int size;
} slab_t;

slab_t cache_16b;
slab_t cache_64b;
slab_t cache_32b;

void slab_init(slab_t* slab, int size)
{
    slab->bitmap = 0;
    slab->size = size;

    slab->first_slot = buddy_alloc(0);
}

void* slab_alloc(slab_t* slab){
  
    uint32_t free_mask=~slab->bitmap;
    if(free_mask==0) return NULL;
    int free_bit=__builtin_ctz(~slab->bitmap);

        slab->bitmap |= (1<<free_bit);
        
        return (void*)((uint32_t)slab->first_slot +(free_bit*slab->size));
    
}

void slab_free(slab_t* slab, void* ptr)
{
    int index = ((uint32_t)ptr - (uint32_t)slab->first_slot) / slab->size;

    slab->bitmap &= ~(1 << index);
}