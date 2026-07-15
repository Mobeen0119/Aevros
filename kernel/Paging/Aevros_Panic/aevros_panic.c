#include "aevros_panic.h"
#include "../../Process/task.h"
#include "../../Memory/KallocTracker/kalloc_tracker.h"
#include "../../Memory/kheap.h"
#include "../../../Lib/kprintf.h"
#include "../../../Include/screen.h"
#include "../../../Include/terminal.h"
#include "../../Process/TaskLife/tasklife.h"
#include <stdint.h>

static inline uint32_t read_cr2(void)
{
    uint32_t v;
    asm volatile("mov %%cr2, %0" : "=r"(v));
    return v;
}

extern uint32_t kernel_end;

static int addr_in_kernel_image(uint32_t addr)
{
    return addr >= 0x100000 && addr <= (uint32_t)&kernel_end;
}


static void print_stack_trace(uint32_t ebp, uint32_t stack_base, uint32_t stack_top)
{
    kprintf(" How the kernel got here (each line called the one above it):\n\n");

    int have_bounds = (stack_base != 0 && stack_top != 0);

    for (int i = 0; i < 8 && ebp; i++)
    {
        if (have_bounds && (ebp < stack_base || ebp >= stack_top))
        {
            set_color(VGA_YELLOW, VGA_BLACK);
            kprintf("   %d. frame pointer 0x%x is outside the expected stack range,\n", i, ebp);
            kprintf("      stopping the walk here.\n");
            reset_color();
            return;
        }

        uint32_t *frame = (uint32_t *)ebp;
        uint32_t ret = frame[1];
        uint32_t next_ebp = frame[0];
        int is_last_real_frame = (next_ebp == 0 || next_ebp <= ebp);

        if (ret != 0 && addr_in_kernel_image(ret))
        {
            kprintf("   %d. called from 0x%x\n", i, ret);
        }
        else if (is_last_real_frame)
        {
            set_color(VGA_LIGHT_GREEN, VGA_BLACK);
            kprintf("   %d. this is the start of this task -- no earlier caller\n", i);
            reset_color();
        }
        else
        {
            set_color(VGA_LIGHT_RED, VGA_BLACK);
            kprintf("   %d. return address 0x%x doesn't point into kernel code --\n", i, ret);
            kprintf("      the stack looks corrupted from here on. Stopping.\n");
            reset_color();
            return;
        }

        if (is_last_real_frame)
            return;

        ebp = next_ebp;
    }
}

void decode_fault(uint32_t err, uint32_t addr,
                         const char **what, const char **verdict)
{
    int protection = err & 0x1;
    int write = err & 0x2;
    int fetch = err & 0x10;

    if (fetch)
        *what = "tried to run code from an address with no code mapped there";
    else if (!write && !protection)
        *what = "tried to read from an address that isn't mapped to any memory";
    else if (write && !protection)
        *what = "tried to write to an address that isn't mapped to any memory";
    else if (write && protection)
        *what = "tried to write to memory that's mapped read-only (or otherwise off-limits)";
    else
        *what = "tried to read memory it isn't allowed to read";

    if (addr < 0x1000)
        *verdict = "This is a classic NULL pointer bug: something used a pointer that was never set (or was explicitly zeroed) before being dereferenced.";
    else if (addr > 0xC0000000 && (err & 0x4))
        *verdict = "A user program reached into kernel-only memory. That's expected to be blocked, and it was -- the program gets terminated, the kernel is fine.";
    else if (addr > 0xFFFFF000)
        *verdict = "The stack pointer went past the top of its stack -- almost always runaway/unbounded recursion, or a stack that was sized too small for what it's doing.";
    else if (!protection)
        *verdict = "The address was never mapped to real memory at all -- likely a wild pointer (uninitialized, already freed, or just plain wrong).";
    else
        *verdict = "The memory exists and is mapped, but the access it tried isn't allowed on it (e.g. writing to something read-only).";
}

static void print_bar(uint32_t used, uint32_t total, const char *label)
{
    if (total == 0) return;
    uint32_t pct = (used * 100) / total;
    kprintf(" %s: %u / %u (%u%%)\n", label, used, total, pct);
}

void aevros_panic(const char *reason, register_t *regs)
{
    asm volatile("cli");

    set_color(VGA_LIGHT_RED, VGA_BLACK);
    kprintf("\n");
    kprintf("###############################################\n");
    kprintf("#         SOMETHING WENT WRONG IN THE KERNEL   #\n");
    kprintf("###############################################\n");
    reset_color();

    kprintf("\n The kernel just hit a fault it can't safely recover from: %s.\n", reason);
    kprintf(" Everything below explains what happened and why, in order.\n");
    render();

    uint32_t cr2 = read_cr2();

    if (regs && regs->int_no == 14)
    {
        const char *what = "";
        const char *verdict = "";
        decode_fault(regs->err_code, cr2, &what, &verdict);

        set_color(VGA_CYAN, VGA_BLACK);
        kprintf("\n-- WHAT HAPPENED --------------------------------\n");
        reset_color();
        kprintf(" The %s code %s\n", (regs->err_code & 0x4) ? "user-mode" : "kernel-mode", what);
        kprintf(" at address 0x%x, while doing a %s.\n",
                cr2, (regs->err_code & 0x2) ? "WRITE" : "READ");
        kprintf("\n Most likely explanation:\n   %s\n", verdict);
        render();
    }
    else
    {
        set_color(VGA_CYAN, VGA_BLACK);
        kprintf("\n-- WHAT HAPPENED --------------------------------\n");
        reset_color();
        kprintf(" The CPU raised a \"%s\" exception -- this is not a page fault,\n", reason);
        kprintf(" so there's no memory address to blame; it's usually a genuine\n");
        kprintf(" bug in the instruction stream itself (bad opcode, divide by\n");
        kprintf(" zero, misconfigured descriptor table, etc).\n");
        render();
    }

    if (regs)
    {
        set_color(VGA_CYAN, VGA_BLACK);
        kprintf("\n-- CPU STATE AT THE MOMENT IT HAPPENED -----------\n");
        reset_color();
        kprintf(" EIP (the exact instruction that faulted): 0x%x\n", regs->eip);
        kprintf(" ESP (top of the stack at that point)     : 0x%x\n", regs->esp);
        kprintf(" EAX=0x%x  EBX=0x%x  ECX=0x%x  EDX=0x%x\n",
                regs->eax, regs->ebx, regs->ecx, regs->edx);
        kprintf(" CS=0x%x  SS=0x%x  (which segment/privilege level it ran in)\n",
                regs->cs, regs->ss);
        render();
    }

    set_color(VGA_CYAN, VGA_BLACK);
    kprintf("\n-- WHICH TASK WAS RUNNING -------------------------\n");
    reset_color();
    if (current_task)
    {
        kprintf(" PID %u (\"%s\"), which had been alive for %u timer ticks.\n",
                current_task->pid, current_task->name,
                get_ticks() - current_task->start_time);
        if (current_task->is_checkpoint_clone)
            kprintf(" (This task is a checkpoint-restored clone.)\n");
    }
    else
    {
        kprintf(" No task was scheduled yet -- this happened during early boot,\n");
        kprintf(" before the kernel finished setting up multitasking.\n");
    }
    render();

    set_color(VGA_CYAN, VGA_BLACK);
    kprintf("\n-- CALL CHAIN --------------------------------------\n");
    reset_color();
    {
        uint32_t base = 0, top = 0;
        if (current_task) { base = current_task->kernel_stack_base; top = current_task->kernel_stack; }
        if (regs)
            print_stack_trace(regs->ebp, base, top);
        else
        {
            uint32_t ebp;
            asm volatile("mov %%ebp, %0" : "=r"(ebp));
            print_stack_trace(ebp, base, top);
        }
    }
    render();

    set_color(VGA_CYAN, VGA_BLACK);
    kprintf("\n-- MEMORY AT THE TIME OF THE CRASH -----------------\n");
    reset_color();
    print_bar(heap_used_bytes(), heap_used_bytes() + heap_free_bytes(), "heap used");
    kprintf(" live allocations still outstanding: %u\n", tracker_live_count());
    if (tracker_leaked_bytes() > 0)
    {
        set_color(VGA_YELLOW, VGA_BLACK);
        kprintf(" %u bytes worth of allocations were never freed (\"ghosts\").\n",
                tracker_leaked_bytes());
        kprintf(" These are listed below -- they're not necessarily related to\n");
        kprintf(" this crash, but they're worth checking:\n");
        reset_color();
        tracker_dump();
    }
    else
    {
        set_color(VGA_LIGHT_GREEN, VGA_BLACK);
        kprintf(" No leaked allocations detected -- memory tracking looks clean.\n");
        reset_color();
    }
    render();

    set_color(VGA_CYAN, VGA_BLACK);
    kprintf("\n-- RECENT TASK HISTORY ------------------------------\n");
    reset_color();
    tasklife_dump_current();

    set_color(VGA_LIGHT_RED, VGA_BLACK);
    kprintf("\n###############################################\n");
    kprintf("# System halted. Nothing above was lost or      #\n");
    kprintf("# corrupted by this report -- it's safe to      #\n");
    kprintf("# power cycle and read this back at your leisure.#\n");
    kprintf("###############################################\n");
    reset_color();
    render();

    for (;;)
        asm volatile("hlt");
}