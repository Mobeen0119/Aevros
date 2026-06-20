#include "GDT.h"
#include "TSS.h"
#include "../../Lib/string.h"
#include "../Memory/pmm.h"
#include "../../Lib/kprintf.h"

struct gdt_entry_struct gdt_entries[6];

extern void gdt_flush(uint32_t gdt_ptr);
struct gdt_ptr_s
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct gdt_ptr_s gdt_ptr;

void gdt_gate_set(int32_t num, uint32_t base, uint32_t limit, uint8_t acc, uint8_t gran)
{
    gdt_entries[num].base_low = base & 0xFFFF;
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high = (base >> 24) & 0xFF;
    gdt_entries[num].limit_low = limit & 0xFFFF;
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
    write_tss(5, 0x10, 0);

    kprintf("GDT1: base_low=%x mid=%x high=%x limit_low=%x flags=%x access=%x\n",
            gdt_entries[1].base_low, gdt_entries[1].base_middle, gdt_entries[1].base_high,
            gdt_entries[1].limit_low, gdt_entries[1].Limit_high_flags, gdt_entries[1].access);

    gdt_ptr.limit = (uint16_t)(sizeof(gdt_entries) - 1);
    gdt_ptr.base = (uint32_t)&gdt_entries;
    gdt_flush((uint32_t)&gdt_ptr);

    gdt_flush((uint32_t)&gdt_ptr);
    load_tss();
}
void update_tss(uint32_t stack)
{
    tss.esp0 = stack;
}