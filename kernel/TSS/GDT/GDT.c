#include <stdint.h>
#include "GDT.h"
#include "../../Memory/pmm.h"

struct gdt_entry_struct gdt_entries[5];
tss_entry_t tss_entry;

struct gdt_ptr_s
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

struct gdt_ptr_s gdt_ptr;

void gdt_gate_set(int32_t num, uint32_t base, uint32_t limit, uint8_t acc, uint8_t gran)
{
    gdt_entries[num].base_low = base & 0xFFFF;
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high = (base >> 24) & 0xFF;

    gdt_entries[num].limit_low = (limit & 0xFFFF);
    gdt_entries[num].Limit_high_flags = (limit >> 16) & 0x0F;

    gdt_entries[num].Limit_high_flags |= gran & 0xF0;
    gdt_entries[num].access = acc;
}

void gdt_init()
{

    memset(gdt_entries, 0, sizeof(gdt_entries));

    gdt_gate_set(0, 0, 0, 0, 0);

    gdt_gate_set(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    gdt_gate_set(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    gdt_gate_set(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);

    gdt_gate_set(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

    gdt_ptr.limit = sizeof(struct gdt_entry_struct) - 1;
    gdt_ptr.base = (uint32_t)gdt_entries;

    gdt_flush((uint32_t)&gdt_ptr);
}

void write_tss(int32_t num, uint32_t ss0, uint32_t esp0)
{
    uint32_t base = (uint32_t)&tss_entry;
    uint32_t limit = sizeof(tss_entry) - 1;

    gdt_gate_set(num, base, limit, 0x89, 0x00);

    memset(&tss_entry, 0, sizeof(tss_entry));

    tss_entry.ss0 = ss0;
    tss_entry.esp0 = esp0;
}

void update_tss(uint32_t stack)
{
    tss_entry.esp0 = stack;
}