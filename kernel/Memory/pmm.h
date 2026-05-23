#include <stdint.h>

uint32_t mem[1024]; // 4KB of memory

void memset(void *dest, uint8_t value, uint32_t size)
{
    uint8_t *ptr = (uint8_t *)dest;
    for (uint32_t i = 0; i < size; i++)
    {
        ptr[i] = value;
    }
}

void *memcpy(void *dest, const void *src, uint32_t size)
{
    uint8_t *d = dest;
    uint8_t *s = src;
    for (uint32_t i = 0; i < size; i++)
    {
        d[i] = s[i];
    }

    return dest;
}

void pmm_free(uint32_t address);
uint32_t pmm_alloc();

void pmm_init(uint32_t st_address, uint32_t size);