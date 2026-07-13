#include "page_fault.h"
#include "paging.h"
#include "../Process/task.h"
#include "../Paging/isr.h"
#include "../../Lib/kprintf.h"
#include "../Process/TaskLife/tasklife.h"
#include "../../Include/terminal.h"
#include "Aevros_Panic/aevros_panic.h"

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
    int user = err & 0x4;

    if (!user)
    {
        aevros_panic("page fault", reg);
        return;
    }

    const char *what = "";
    const char *verdict = "";
    decode_fault(err, addr, &what, &verdict);

    set_color(VGA_YELLOW, VGA_BLACK);
    kprintf("\n===============================================\n");
    kprintf(" A PROGRAM CRASHED (the rest of the system is fine)\n");
    kprintf("===============================================\n");
    reset_color();

    kprintf("\n Program PID %u %s\n", current_task ? current_task->pid : 0, what);
    kprintf(" at address 0x%x (this was a %s).\n",
            addr, (err & 0x2) ? "write" : "read");
    kprintf("\n Most likely explanation:\n   %s\n", verdict);
    kprintf("\n Where it happened: EIP=0x%x  ESP=0x%x  CS=0x%x  SS=0x%x\n",
            reg->eip, reg->esp, reg->cs, reg->ss);

    set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    kprintf("\n This program is being shut down. Nothing else on the system\n");
    kprintf(" is affected -- user-mode crashes are contained to the one\n");
    kprintf(" program that caused them.\n");
    reset_color();
    render();

    if (current_task)
    {
        task_log_event(current_task, EVT_FAULT, addr);
        sys_exit(-1);
    }
    for (;;)
        asm volatile("hlt");
}