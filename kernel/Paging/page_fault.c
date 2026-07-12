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

    set_color(VGA_RED, VGA_BLACK);
    kprintf("\n--------------PAGE FAULT---------------\n");
    kprintf("Address  : 0x%x\n", addr);
    kprintf("What     : %s\n", what);
    kprintf("Verdict  : %s\n", verdict);
    kprintf("Access   : %s\n", (err & 0x2) ? "Write" : "Read");
    kprintf("Mode     : User\n");
    kprintf("EIP=%x ESP=%x CS=%x SS=%x\n", reg->eip, reg->esp, reg->cs, reg->ss);
    kprintf("\n----USER PROCESS CRASH: killing pid %u----\n",
            current_task ? current_task->pid : 0);
    reset_color();

    if (current_task)
    {
        task_log_event(current_task, EVT_FAULT, addr);
        sys_exit(-1);
    }
    for (;;)
        asm volatile("hlt");
}