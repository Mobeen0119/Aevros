#pragma once
#include <stdint.h>

struct gdt_entry_struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  Limit_high_flags;
    uint8_t  base_high;
} __attribute__((packed));

extern struct gdt_entry_struct gdt_entries[6];

void gdt_gate_set(int32_t num, uint32_t base, uint32_t limit, uint8_t acc, uint8_t gran);
void gdt_init();
void update_tss(uint32_t stack);