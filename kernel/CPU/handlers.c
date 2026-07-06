#include "../../Lib/kprintf.h"
#include "../Process/task.h"
#include "../io.h"
#include "../../Drivers/keyboard.h"
#include "../../Drivers/PIT/pit.h"
#include "../../Lib/kprintf.h"

void irq_handler(register_t *r)
{
    
    if (r->int_no >= 40)
        outb(0xA0, 0x20);
    outb(0x20, 0x20);    

    if (r->int_no == 32)
        timer_callback(r);

    else if (r->int_no == 33)
        keyboard_handler();
}