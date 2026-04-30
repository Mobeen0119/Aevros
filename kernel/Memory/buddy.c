#include <stdint.h>
#include <stdbool.h>
#include "pmm.c"

#define MAX_ORDER 10

typedef struct _buddy_block
{
    struct _buddy_block *next; // Pointer to the next block in the free list               
} buddy_block_t;

buddy_block_t *free_lists[MAX_ORDER + 1];
   
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
buddy_block_t *buddy = (buddy_block_t *)((uintptr_t)block + size);                add_to_list(buddy, i);
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