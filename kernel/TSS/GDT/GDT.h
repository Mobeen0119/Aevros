#include <stdint.h>

struct gdt_entry_struct
{
    uint16_t base_low, limit_low;
    uint8_t base_middle, base_high, access, Limit_high_flags;
} __attribute__((packed));

struct tss_entry_struct
{
    uint32_t prev_tss; // If we used hardware task switching
    uint32_t esp0;     // The stack pointer to load when  switch to Ring 0
    uint32_t ss0;      // The stack segment to load when  switch to Ring 0
    uint32_t esp1;     // Everything below here is unused by us,
    uint32_t ss1, esp2, ss2, cr3, eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi, es, cs, ss, ds, fs, gs, ldt;
    uint16_t trap,iomap_base;
} __attribute__((packed));

typedef struct tss_entry_struct tss_entry_t;

void gdt_init();
