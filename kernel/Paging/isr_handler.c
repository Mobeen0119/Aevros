#include "isr.h"
#include <stdint.h>
#include "../Memory/pmm.h"
#include "../Process/task.h"
#include "page_fault.h"
#include "../../Lib/kprintf.h"
#include "../../Drivers/keyboard.h"
#include "../io.h"
#include "../Paging/paging.h"
#include "../../Drivers/PIT/pit.h"

#include "Aevros_Panic/aevros_panic.h"

void isr_handler(struct registers *r)
{
    if (r->int_no == 14)
    {
        page_fault_handler(r);
        return;
    }

    if (r->int_no == 32)
    {
        outb(0x20, 0x20);
        timer_callback(r);
        return;
    }




    if (r->int_no == 33)
    {
        outb(0x20, 0x20);
        keyboard_handler();
        return;
    }

    if (r->int_no < 32)
    {
        static const char *exceptions[] = {
            "divide by zero",
            "debug",
            "NMI",
            "breakpoint",
            "overflow",
            "bound range exceeded",
            "invalid opcode",
            "device not available",
            "double fault",
            "coprocessor segment overrun",
            "invalid TSS",
            "segment not present",
            "stack segment fault",
            "general protection fault",
            "page fault",
            "reserved",
            "x87 float fault",
            "alignment check",
            "machine check",
            "SIMD float fault"};

        const char *name = (r->int_no < 20)
                               ? exceptions[r->int_no]
                               : "unknown exception";

        aevros_panic(name, r);
        return;
    }

    if (r->int_no >= 40)
        outb(0xA0, 0x20);

    outb(0x20, 0x20);
}
