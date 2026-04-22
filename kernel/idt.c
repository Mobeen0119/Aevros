#include "idt.h"

struct IDT_entry idt[256];
struct IDT_ptr idtp;

extern void idt_load(unsigned int);

void idt_gate_set(int n, unsigned int handler)
{
    idt[n].zero = 0;
    idt[n].offset_high = (handler >> 16) & 0xFFFF;
    idt[n].offset_low = handler & 0xFFFF;
    idt[n].type_attr = 0x8E;
    idt[n].selector = 0x08;
}

void idt_init()
{
    idtp.limit = (sizeof(struct IDT_entry) * 256) - 1;
    idtp.base = (unsigned int)&idt;

    idt_load((unsigned int)&idtp);
}
