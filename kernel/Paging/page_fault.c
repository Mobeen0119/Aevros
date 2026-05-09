#include "page_fault.h"
#include "paging.h"
#include "../Process/task.h"
#include "../Paging/isr.h"


static inline uint32_t read_cr2()
{
    uint32_t value;
    asm volatile("mov %%cr3,%0" : "=r"(value));
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

    kprintf("\n--------------PAGE FAULT---------------\n");

    kprintf("Address : 0s%x\n", addr);

    kprintf("Cause of it : %x\n", protection ? "Protection Violation " : "Page Not Present");

    kprintf("Access : %s\n", write ? "Write" : "Read");

    kprintf("Mode : %s\n", user ? "User" : "Kernel");

    if (reserved)
        kprintf("Reserved bits overwritten\n");
    if (fetch)
        kprintf("Instruction Fetch Fault\n");

        //Future----
    kprintf("ESP : 0x%x\n", reg->esp);
    kprintf("EIP : 0x%x\n", reg->eip);

    if(user){
    kprintf("\n----USER PROCESS CRASH\n");
    for (;;)
        asm volatile ("hlt"); 
    }


    kprintf("\nKERNEL PANIC\n");
    for(;;) asm volatile ("hlt");
}