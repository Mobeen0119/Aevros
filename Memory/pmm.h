#include <stdint.h>

uint32_t mem[1024];      // 4KB of memory

void memset(void* dest, uint8_t value, uint32_t size)
{
    uint8_t* ptr = (uint8_t*)dest;
    for (uint32_t i = 0; i < size; i++)
    {
        ptr[i] = value;
    }
}
