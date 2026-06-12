#include "page_fault.h"
#include "paging.h"
#include "../Process/task.h"
#include "../Paging/isr.h"
#include "../../Lib/kprintf.h"

static inline uint32_t read_cr2()
{
    uint32_t value;
    asm volatile("mov %%cr2,%0" : "=r"(value));
    return value;
}

void page_fault_handler(struct registers *reg)
{
    uint32_t fault_addr;
    asm volatile("mov %%cr2, %0" : "=r"(fault_addr));

    kprintf("PAGE FAULT!\n");
    kprintf("CR2 = %x\n", fault_addr);
    kprintf("EIP = %x\n", reg->eip);
    kprintf("ESP = %x\n", reg->esp);

    uint32_t addr = read_cr2();
    uint32_t err = reg->err_code;

    int protection = err & 0x1;
    int write = err & 0x2;
    int user = err & 0x4;
    int reserved = err & 0x8;
    int fetch = err & 0x10;

    kprintf("\n--------------PAGE FAULT---------------\n");

    kprintf("Address : 0s%x\n", addr);

kprintf("Cause of it : %s\n",protection ? "Protection Violation": "Page Not Present");
    kprintf("Access : %s\n", write ? "Write" : "Read");

    kprintf("Mode : %s\n", user ? "User" : "Kernel");

    if (reserved)
        kprintf("Reserved bits overwritten\n");
    if (fetch)
        kprintf("Instruction Fetch Fault\n");
    

    // Future----
    // kprintf("ESP : 0x%x\n", reg->esp);
    // kprintf("EIP : 0x%x\n", reg->eip);
    kprintf("CS=%x\n", reg->cs);
kprintf("SS=%x\n", reg->ss);

    if (user)
    {
        kprintf("\n----USER PROCESS CRASH\n");
        for (;;)
            asm volatile("hlt");
    }

    kprintf("\nKERNEL PANIC\n");
    for (;;)
        asm volatile("hlt");
}