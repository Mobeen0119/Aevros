#include "isr.h"

void isr_handler(struct registers r)
{
    if (r.int_no == 33)
    {
        keyboard_handler();
    }
    else if (r.int_no < 32)
    {
        print_string("CPU HANDLES IT : ");
        print_hex(r.int_no);
        asm volatile("hlt");
    }

    if (r.int_no > 32)
        outb(0x20, 0x20);
}