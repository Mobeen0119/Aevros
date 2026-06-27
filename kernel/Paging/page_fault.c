#include "page_fault.h"
#include "paging.h"
#include "../Process/task.h"
#include "../Paging/isr.h"
#include "../../Lib/kprintf.h"
#include "../Process/TaskLife/tasklife.h"
#include "../io.h"


static inline uint32_t read_cr2()
{
    uint32_t value;
    asm volatile("mov %%cr2,%0" : "=r"(value));
    return value;
}

void page_fault_handler(struct registers *reg)
{
    outb(0xE9, 'P');

    outb(0xE9, '{');
    dbg_hex32(read_cr2());        // fault address
    dbg_hex32(reg->err_code);     // error code
    dbg_hex32(reg->eip);
    dbg_hex32(reg->useresp);
    dbg_hex32(reg->ebp);
    outb(0xE9, '}');

    kprintf("PAGE FAULT!\n");
    outb(0xE9, '2');

    kprintf("EIP = %x\n", reg->eip);
    outb(0xE9, '3');
    kprintf("ESP = %x\n", reg->esp);
    outb(0xE9, '4');

    uint32_t addr = read_cr2();
    outb(0xE9, '5');
    uint32_t err = reg->err_code;
    outb(0xE9, '6');

    int protection = err & 0x1;
    int write = err & 0x2;
    int user = err & 0x4;
    int reserved = err & 0x8;
    int fetch = err & 0x10;
    outb(0xE9, '7');

    kprintf("\n--------------PAGE FAULT---------------\n");
    outb(0xE9, '8');
    kprintf("Address : 0x%x\n", addr);
    outb(0xE9, '9');
    kprintf("Cause of it : %s\n", protection ? "Protection Violation" : "Page Not Present");
    outb(0xE9, 'a');
    kprintf("Access : %s\n", write ? "Write" : "Read");
    outb(0xE9, 'b');
    kprintf("Mode : %s\n", user ? "User" : "Kernel");
    outb(0xE9, 'c');

    if (reserved)
        kprintf("Reserved bits overwritten\n");
    outb(0xE9, 'd');
    if (fetch)
        kprintf("Instruction Fetch Fault\n");
    outb(0xE9, 'e');

    kprintf("CS=%x\n", reg->cs);
    outb(0xE9, 'f');
    kprintf("SS=%x\n", reg->ss);
    outb(0xE9, 'g');

    if (user)
    {
        outb(0xE9, 'U');
        kprintf("\n----USER PROCESS CRASH\n");
        outb(0xE9, 'V');
        if (current_task)
        {
            outb(0xE9, 'W');
            task_log_event(current_task, EVT_FAULT, addr);
            outb(0xE9, 'X');
            outb(0xE9, '.'); 
            outb(0xE9, '?');
            sys_exit(-1);
            outb(0xE9, 'Y');
        }
        outb(0xE9, 'Z');
        for (;;)
            asm volatile("hlt");
    }

    if (current_task)
    {
        task_log_event(current_task, EVT_FAULT, addr);
    }
    kprintf("\nKERNEL PANIC\n");
    for (;;)
        asm volatile("hlt");
}
