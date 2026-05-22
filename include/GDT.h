#include <stdint.h>

struct gdt_entry_struct
{
    uint16_t base_low, limit_low;
    uint8_t base_middle, base_high, access, Limit_high_flags;
} __attribute__((packed));


void gdt_gate_set(int32_t num, uint32_t base, uint32_t limit, uint8_t acc, uint8_t gran);
void gdt_init();



void gdt_init();
