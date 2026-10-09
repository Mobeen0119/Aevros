#include <stdint.h>

uint32_t get_ticks(void)
{
    static uint32_t ticks = 0;
    return ticks++;
}
