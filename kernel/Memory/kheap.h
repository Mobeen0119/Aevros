#include <stdint.h>
#include <stddef.h>



int size_to_order(size_t size);

void *kmalloc(size_t size);

void kfree(uint32_t *ptr);