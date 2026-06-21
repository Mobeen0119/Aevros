#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "pmm.h"
#include "buddy.h"

static uint32_t buddy_total_mem = 0;

buddy_block_t *free_lists[MAX_ORDER + 1];

void buddy_init(uint32_t start, uint32_t end)
{

    for (int i = 0; i <= MAX_ORDER; i++)
        free_lists[i] = NULL;

    uint32_t block_size = (1 << MAX_ORDER) * 4096;
    uint32_t addr = start;
    buddy_total_mem = end - start;

    if (addr & (block_size - 1))
        addr = (addr + block_size-1) & ~(block_size - 1);

    while (addr + block_size <= end)
    {
        add_to_list((void *)addr, MAX_ORDER);
        addr += block_size;
    }

    if (free_lists[MAX_ORDER] == NULL)
    {
        addr = start;
        while (addr + 4096 <= end)
        {
            add_to_list((void *)addr, 0);
            addr += 4096;
        }
    }
}

void *buddy_alloc(int order)
{
    if (free_lists[order])
    {
        buddy_block_t *block = free_lists[order];
        free_lists[order] = block->next;
        return (void *)block;
    }

    for (int i = order + 1; i <= MAX_ORDER; i++)
    {
        if (free_lists[i] != NULL)
        {
            void *block = (void *)free_lists[i];

            remove_from_list(block, i);

            while (i > order)
            {
                i--;
                uint32_t size = (1 << i) * 4096;
                buddy_block_t *buddy = (buddy_block_t *)((uintptr_t)block + size);
                add_to_list(buddy, i);
            }
            return (void *)block;
        }
    }
    return NULL;
}

void add_to_list(void *ptr, int order)
{
    buddy_block_t *block = (buddy_block_t *)ptr;
    block->next = free_lists[order];
    free_lists[order] = block;
}

void remove_from_list(void *ptr, int order)
{
    buddy_block_t *target = (buddy_block_t *)ptr;
    buddy_block_t *current = free_lists[order];

    if (!current || !target)
        return;

    if (current == target)
    {
        free_lists[order] = current->next;
        return;
    }
    while (current && current->next != target)
        current = current->next;

    if (current && current->next == target)
        current->next = target->next;
}

void buddy_free(void *ptr, int order)
{
    uintptr_t address = (uintptr_t)ptr;
    uintptr_t size = ((uintptr_t)1 << order) * 4096;

    while (order < MAX_ORDER)
    {
        uintptr_t buddy_address = address ^ size;

        buddy_block_t *current = free_lists[order];
        buddy_block_t *prev = NULL;

        bool found = false;

        while (current)
        {
            if ((uintptr_t)current == buddy_address)
            {
                found = true;
                break;
            }
            prev = current;
            current = current->next;
        }

        if (!found)
            break;

        if (prev)
            prev->next = current->next;
        else
            free_lists[order] = current->next;

        address &= ~size;

        order++;
        size <<= 1;
    }

    add_to_list((void *)address, order);
}

uint32_t buddy_total_memory(void)
{
    return buddy_total_mem;
}

uint32_t buddy_free_memory(void)
{
    uint32_t free = 0;

    for (int order = 0; order <= MAX_ORDER; order++)
    {
        buddy_block_t *curr = free_lists[order];
        uint32_t block_bytes = ((uint32_t)1 << order) * 4096;

        while (curr)
        {
            if (free > UINT32_MAX - block_bytes)
            {
                free = UINT32_MAX;
                break;
            }
            free += block_bytes;
            curr = curr->next;
        }
    }
    return free;
}

uint32_t buddy_used_memory(void)
{
    uint32_t free = buddy_free_memory();
    return (buddy_total_mem >= free) ? buddy_total_mem - free : 0;
}

uint32_t buddy_largest_free_block(void)
{
    for (int order = MAX_ORDER; order >= 0; order--)
    {
        if (free_lists[order])
            return ((uint32_t)1 << order) * 4096;
    }
    return 0;
}

uint32_t buddy_fragmentation(void)
{
    uint32_t free = buddy_free_memory();
    uint32_t largest = buddy_largest_free_block();

    return (free > largest) ? free - largest : 0;
}

uint32_t buddy_free_count_at_order(int order){
    if(order <0 || order > MAX_ORDER) return 0;

    uint32_t count=0;
    
    buddy_block_t* b=free_lists[order];

    while(b) {
         count++;
         b=b->next;
    }
    return count;
}

uint32_t buddy_live_allocations(void)
{
    uint32_t total_pages = buddy_total_mem / 4096;
    uint32_t free_pages  = 0;

    for (int order = 0; order <= MAX_ORDER; order++)
        free_pages += buddy_free_count_at_order(order) * ((uint32_t)1 << order);

    return total_pages - free_pages;
}

