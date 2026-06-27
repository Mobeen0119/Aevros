#include "forge_panic.h"
#include "../../Process/task.h"
#include "../../Memory/KallocTracker/kalloc_tracker.h"
#include "../../Memory/kheap.h"
#include "../../../Lib/kprintf.h"
#include "../../../Include/screen.h"
#include "../../Process/TaskLife/tasklife.h"
#include <stdint.h>

static inline uint32_t read_cr2(void)
{
    uint32_t v;
    asm volatile("mov %%cr2, %0" : "=r"(v));

    return v;
}

static void print_stack_trace(uint32_t ebp)
{
    kprint("---stack trace : \n");

    for (int i = 0; i < 8 && ebp; i++)
    {
        uint32_t *frame = (uint32_t *)ebp;
        uint32_t ret = frame[1];
        if (!ret)
            break;
        kprintf("     #%d    0x%x\n ", i, ret);
        ebp = frame[0];
    }
}

static void decode_fault(uint32_t err, uint32_t addr,
                         const char **what, const char **verdict)
{

    int protection = err & 0x1;
    int write = err & 0x2;
    int fetch = err & 0x10;

    if (fetch)
        *what = " Instruction fetch from from unmapped address";
    else if (!write && !protection)
        *what = "read from unmapped address";
    else if (write && protection)
        *what = "write to protected address";
    else
        *what = "read from protected address";

    if (addr < 0x1000)
        *verdict = "NULL pointer dereference";
    else if (addr > 0xC0000000 && (err & 0x4))
        *verdict = "user process accessed kernel memory";
    else if (addr > 0xFFFFF000)
        *verdict = "stack overflow ... esp past top of stack";
    else if (!protection)
        *verdict = "pointer to never-allocated memory";
    else
        *verdict = "access violation";
}

void forge_panic(const char *reason, register_t *regs)
{
    asm volatile("cli");

    uint32_t cr2 = read_cr2();
    kprintf("\n");
    kprintf("╔══════════════════════════════════════════╗\n");
    kprintf("║         FORGE OS  ---  KERNEL FAULT      ║\n");
    kprintf("╠══════════════════════════════════════════╣\n");

    kprintf("|| REASON    : %-31s||\n ", reason);
    if (regs && regs->int_no == 14)
    {
        const char *what = "";
        const char *verdict = "";
        decode_fault(regs->err_code, cr2, &what, &verdict);

        kprintf("║ FAULT   : 0x%x\n", cr2);
        kprintf("║ WHAT    : %s\n", what);
        kprintf("║ VERDICT : %s\n", verdict);
        kprintf("║ ACCESS  : %s\n",
                (regs->err_code & 0x2) ? "WRITE" : "READ");
        kprintf("║ MODE    : %s\n",
                (regs->err_code & 0x4) ? "USER" : "KERNEL");
    }
    if (regs)
    {
        kprintf("╠══════════════════════════════════════════╣\n");
        kprintf("║ REGISTERS                                ║\n");
        kprintf("║  EIP=0x%x  ESP=0x%x\n", regs->eip, regs->esp);
        kprintf("║  EAX=0x%x  EBX=0x%x\n", regs->eax, regs->ebx);
        kprintf("║  ECX=0x%x  EDX=0x%x\n", regs->ecx, regs->edx);
        kprintf("║  CS=0x%x   SS=0x%x\n", regs->cs, regs->ss);
    }
    kprintf("╠══════════════════════════════════════════╣\n");
    if (current_task)
    {
        kprintf("║ PROCESS : pid %-4u  state %-15u║\n",
                current_task->pid, current_task->state);
        kprintf("║ UPTIME  : %u ticks since created\n",
                get_ticks() - current_task->start_time);
    }
    else
    {
        kprintf("║ PROCESS : none ... fault in early kernel   ║\n");
    }
    kprintf("╠══════════════════════════════════════════╣\n");
    kprintf("║ STACK TRACE                              ║\n");
    if (regs)
        print_stack_trace(regs->ebp);
    else
    {
        uint32_t ebp;
        asm volatile("mov %%ebp, %0" : "=r"(ebp));
        print_stack_trace(ebp);
    }

    kprintf("╠══════════════════════════════════════════╣\n");
    kprintf("║ MEMORY                                   ║\n");
    kprintf("║  heap used  : %u bytes\n", heap_used_bytes());
    kprintf("║  heap free  : %u bytes\n", heap_free_bytes());
    kprintf("║  live allocs: %u\n", tracker_live_count());
    kprintf("║  ghost bytes: %u\n", tracker_leaked_bytes());

    kprintf("╠══════════════════════════════════════════╣\n");
    kprintf("║ GHOST ALLOCATIONS AT PANIC               ║\n");
    tracker_dump();

    kprintf("╠════════════════════════════════════════════╣\n");
    kprintf("║  System Halted ... no data lost in tracker ║\n");
    kprintf("╚════════════════════════════════════════════╝\n");

    kprintf("╠══════════════════════════════════════════╣\n");
    kprintf("║ TASK LIFECYCLE                           ║\n");
    tasklife_dump_current();

    for (;;)
        asm volatile("hlt");
}
