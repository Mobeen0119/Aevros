#ifndef IDT_H
#define IDT_H

struct IDT_entry
{
    unsigned short offset_low;
    unsigned short selector;
    unsigned char zero;
    unsigned char type_attr;
    unsigned short offset_high;

} __attribute__((packed));

struct IDT_ptr
{
    unsigned short limit;
    unsigned int base;
} __attribute__((packed));

void idt_init();
void idt_gate_set(int n, unsigned int handler);

#endif
