#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "pmm.h"
#include "buddy.h"
#include "../../Lib/kprintf.h"

static uint32_t buddy_total_mem = 0;
static uint32_t buddy_start = 0;
static uint32_t buddy_end = 0;

buddy_block_t *free_lists[MAX_ORDER + 1];

static int buddy_in_range(void *ptr)
{
    uintptr_t p = (uintptr_t)ptr;
    return (p >= buddy_start && p < buddy_end);
}

static int buddy_valid_order(int order)
{
    return (order >= 0 && order <= MAX_ORDER);
}

void buddy_init(uint32_t start, uint32_t end)
{
    buddy_start = start;
    buddy_end = end;

    for (int i = 0; i <= MAX_ORDER; i++)
        free_lists[i] = NULL;

    buddy_total_mem = end - start;

    int order = MAX_ORDER;
    uint32_t addr = start;
    uint32_t block_size = (1u << order) * 4096;

    while (order > 0)
    {
        uint32_t aligned = (addr + block_size - 1) & ~(block_size - 1);
        if (aligned + block_size <= end)
        {
            addr = aligned;
            break;
        }
        order--;
        block_size = (1u << order) * 4096;
    }

    if (order == 0)
    {
        addr = (start + 4095) & ~4095u;
        if (addr + 4096 > end)
            return;
    }

    add_to_list((void *)addr, order);
}

void buddy_debug_list(int order)
{
    buddy_block_t *b = free_lists[order];
    uint32_t count = 0;
    const uint32_t LIMIT = 100000;

    kprintf("order %d list: head=0x%x\n", order, (uint32_t)b);

    while (b && count < LIMIT)
    {
        if (!buddy_in_range(b))
        {
            kprintf("  [%u] 0x%x  <-- OUT OF RANGE!\n", count, (uint32_t)b);
            break;
        }
        b = b->next;
        count++;
    }

    if (count >= LIMIT)
        kprintf("order %d: hit LIMIT (%u) -- list is cyclic or absurdly long\n", order, LIMIT);
    else
        kprintf("order %d: %u nodes, terminated cleanly\n", order, count);
}

void *buddy_alloc(int order)
{
    if (!buddy_valid_order(order))
        return NULL;

    if (free_lists[order])
    {
        buddy_block_t *block = free_lists[order];
        free_lists[order] = block->next;
        block->next = NULL;
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
    if (!ptr || !buddy_valid_order(order))
        return;
    if (!buddy_in_range(ptr))
        return;

    buddy_block_t *block = (buddy_block_t *)ptr;
    block->next = free_lists[order];
    free_lists[order] = block;
}

void remove_from_list(void *ptr, int order)
{
    if (!ptr || !buddy_valid_order(order))
        return;

    buddy_block_t *target = (buddy_block_t *)ptr;
    buddy_block_t *current = free_lists[order];

    if (!current)
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
    if (!ptr || !buddy_valid_order(order))
        return;
    if (!buddy_in_range(ptr))
        return;

    uintptr_t address = (uintptr_t)ptr;
    uintptr_t size = ((uintptr_t)1 << order) * 4096;

    if (address & (size - 1))
        return;

    while (order < MAX_ORDER)
    {
        uintptr_t buddy_address = address ^ size;

        if (buddy_address < buddy_start || buddy_address >= buddy_end)
            break;

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

