
#include "../../Lib/kprintf.h" 
#include "../Process/task.h"


void irq_handler( register_t *r) {
    if (r->int_no >= 40) {
    }

    if (r->int_no == 32) {
        kprint("Timer IRQ received!\n");
        schedule();
    } else if (r->int_no == 33) {
        kprint("Keyboard IRQ received!\n");
    }
}

