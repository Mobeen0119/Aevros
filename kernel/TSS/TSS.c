#include "TSS.h"
#include "../../Include/GDT.h"
#include "../../LIB/string.c"

tss_entry_t tss;

void write_tss(int num, uint16_t ss0, uint32_t esp0)
{
    uint32_t base = (uint32_t)&tss;
    uint32_t limit = sizeof(tss_entry_t) - 1;

    gdt_gate_set(num, base, limit, 0x89, 0x00);

    memset(&tss, 0, sizeof(tss_entry_t));

    tss.ss0 = ss0;
    tss.esp0 = esp0;
    tss.cs = 0x1B;
    tss.ss = tss.ds = tss.es = tss.fs = tss.gs = 0x23;
    tss.iomap_base=sizeof(tss_entry_t);
}