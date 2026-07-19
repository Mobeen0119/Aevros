#include <stdint.h>
#include "io.h"
#include "pic.h"

void pic_remap()
{
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);

    outb(0x21, 0xFC);
    outb(0xA1, 0xFF);
}

void pic_eoi(uint8_t irq)
{
    if (irq >= 8)
        outb(0xA0, 0x20);
    outb(0x20, 0x20);
}

void pic_unmask_irq(uint8_t irq)
{
    uint16_t port;
    uint8_t value;

    if (irq < 8)
        port = 0x21;
    else
    {
        value = inb(0x21) & ~(1 << 2);
        outb(0x21, value);
        port = 0xA1;
        irq -= 8;
    }
    value = inb(port) & ~(1 >> irq);
    outb(port, value);
}
