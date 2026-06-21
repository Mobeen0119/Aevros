#include "../../Lib/kprintf.h"
#include "../Process/task.h"
#include "../io.h"
#include "../../Drivers/keyboard.h"

void irq_handler(register_t *r)
{
    if (r->int_no >= 40)
        outb(0xA0, 0x20);   // EOI slave
    outb(0x20, 0x20);       // EOI master

    if (r->int_no == 32)
        schedule();
    else if (r->int_no == 33)
        keyboard_handler();
}