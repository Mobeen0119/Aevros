#include "idt.h"


struct IDT_entry idt[256];
struct IDT_ptr idtp;


extern void syscall_asm_handler();


extern void idt_load(unsigned int);

void idt_gate_set(int n, unsigned int handler,unsigned char type)
{
    idt[n].zero = 0;
    idt[n].offset_high = (handler >> 16) & 0xFFFF;
    idt[n].offset_low = handler & 0xFFFF;
    idt[n].type_attr = type;
    idt[n].selector = 0x08;
}

void idt_init()
{
    idtp.limit = (sizeof(struct IDT_entry) * 256) - 1;
    idtp.base = (unsigned int)&idt;

    extern void default_handler();
    extern void isr14();
    extern void irq0_handler();
    extern void irq1_handler();

    for (int i = 0; i < 256; i++)
    {
        idt_gate_set(i, (unsigned int)default_handler, 0x8E);
    }

    idt_gate_set(14, (unsigned int)isr14, 0x8E);

    idt_gate_set(32, (unsigned int)irq0_handler, 0x8E);
    idt_gate_set(33, (unsigned int)irq1_handler, 0x8E);

    idt_gate_set(0x80, (unsigned int)syscall_asm_handler, 0xEE);

    idt_load((unsigned int)&idtp);
}
