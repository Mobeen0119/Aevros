#include <stdint.h>
#include <stddef.h>



int size_to_order(size_t size);

void *kmalloc(size_t size);

void kfree(void *ptr);

uint32_t heap_used_bytes(void);
uint32_t heap_free_bytes(void);

uint32_t heap_total_bytes(void);
uint32_t heap_largest_free_block(void);
uint32_t heap_live_allocation(void);