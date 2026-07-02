#include "TSS.h"
#include "GDT.h"
#include "../../Lib/string.h"
#include "../Memory/pmm.h"


tss_entry_t tss;

void write_tss(int32_t num, uint16_t ss0, uint32_t esp0) {
    uint32_t base  = (uint32_t)&tss;
    uint32_t limit = sizeof(tss_entry_t) - 1;

    gdt_gate_set(num, base, limit, 0x89, 0x00);

    memset(&tss, 0, sizeof(tss_entry_t));
    tss.ss0  = ss0;
    tss.esp0 = esp0;
    tss.cs   = 0x0B;   // user code segment 
    tss.ss = tss.ds = tss.es = tss.fs = tss.gs = 0x13; // user data
}