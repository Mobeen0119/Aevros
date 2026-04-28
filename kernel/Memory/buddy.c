
#include <stdint.h>

extern int MAX_ORDER;
void buddy_free(void *ptr, int order)
{
    uint32_t address = (uint32_t)ptr;
    uint32_t size = (1 << order) * 4096;

    while (order < MAX_ORDER)
    {
        uint32_t buddy_address = address ^ size;

        if (!is_buddy_free(buddy_address, order))
            break;

        remove_from_list(buddy_address, order);
        address &= ~size;
        order++;
        size <<= 1;
    }
    add_to_list(address, order);
}