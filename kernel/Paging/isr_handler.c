#include "isr.h"
#include <stdint.h>
#include "../Memory/pmm.c"
#include "../Process/task.c"

void isr_handler(struct registers r)
{
    if (r.int_no == 33)
    {
        keyboard_handler();
    }
    else if (r.int_no == 32)
    {
        schedule();
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

void Paging_handler(struct registers r)
{
    uint32_t faulting_address;
    asm volatile(
        "mov %%cr2,%0" : "=r"(faulting_address));

    int present = !(r.err_code & 0x1);
    int write = r.err_code & 0x2;
    int user_mode = r.err_code & 0x4;

    if (present)
    {
        uint32_t page = pmm_alloc();
        map_page(faulting_address, page, 0x3);
        return;
    }

    if (write && !user_mode)
    {
        print_string("SECURITY VIOLATION: KERNEL PROTECTION FAULT");
        asm volatile("hlt");
    }
}
