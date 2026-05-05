#include <stdint.h>

struct gdt_entry_struct
{
    uint16_t base_low, limit_low;
    uint8_t base_middle, base_high, access, Limit_high_flags;
} __attribute__((packed));


void gdt_init();
