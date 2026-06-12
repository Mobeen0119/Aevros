#include <stdint.h>
#include <stddef.h>



int size_to_order(size_t size);

void *kmalloc(size_t size);

void kfree(void *ptr);

uint32_t heap_used_bytes();
uint32_t heap_free_bytes();