#include "page_fault.h"
#include "paging.h"
#include "../Process/task.h"
#include "../Paging/isr.h"
#include "../../Lib/kprintf.h"
#include "../Process/TaskLife/tasklife.h"
#include "../../Include/terminal.h"

static inline uint32_t read_cr2()
{
    uint32_t value;
    asm volatile("mov %%cr2,%0" : "=r"(value));
    return value;
}

void page_fault_handler(struct registers *reg)
{
    uint32_t addr = read_cr2();
    uint32_t err = reg->err_code;

    int protection = err & 0x1;
    int write = err & 0x2;
    int user = err & 0x4;
    int reserved = err & 0x8;
    int fetch = err & 0x10;

    set_color(VGA_RED, VGA_BLACK);

    kprintf("\n--------------PAGE FAULT---------------\n");
    kprintf("Address : 0x%x\n", addr);
    kprintf("Cause of it : %s\n", protection ? "Protection Violation" : "Page Not Present");
    kprintf("Access : %s\n", write ? "Write" : "Read");
    kprintf("Mode : %s\n", user ? "User" : "Kernel");

    if (reserved)
        kprintf("Reserved bits overwritten\n");
    if (fetch)
        kprintf("Instruction Fetch Fault\n");

    kprintf("EIP=%x\n", reg->eip);
    kprintf("ESP=%x\n", reg->esp);
    kprintf("CS=%x\n", reg->cs);
    kprintf("SS=%x\n", reg->ss);

    if (user)
    {
        kprintf("\n----USER PROCESS CRASH\n");
        reset_color();
        if (current_task)
        {
            task_log_event(current_task, EVT_FAULT, addr);
            sys_exit(-1);
        }
        for (;;)
            asm volatile("hlt");
    }

    if (current_task)
        task_log_event(current_task, EVT_FAULT, addr);

    kprintf("\nKERNEL PANIC\n");
    for (;;)
        asm volatile("hlt");
}
