#ifndef KHEAP_H
#define KHEAP_H

#include <stdint.h>
#include <stddef.h>
#include "KallocTracker/kalloc_tracker.h"

int size_to_order(size_t size);

void *kmalloc_raw(size_t size);

void kfree_raw(void *ptr);

#define kmalloc(size)                                                             \
    ({ void *_p = kmalloc(raw(size);                                              \
                          tracker_record(_p, size, __FILE__, __LINE__, __func__); \
                          _p; })

#define kfree(ptr) \
    ({ tracker_remove(ptr); \
            kfree_raw(ptr); })

uint32_t heap_used_bytes(void);
uint32_t heap_free_bytes(void);

uint32_t heap_total_bytes(void);
uint32_t heap_largest_free_block(void);
uint32_t heap_live_allocation(void);

#endif