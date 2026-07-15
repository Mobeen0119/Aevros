#include "stackmap.h"
#include "../task.h"
#include "../../../Lib/kprintf.h"
#include "../../../Lib/string.h"
#include "../../../Include/terminal.h"

#define STACKMAP_MAX_FRAMES 12

extern uint32_t kernel_end;

static int addr_in_kernel_image(uint32_t addr)
{
    return addr >= 0x100000 && addr <= (uint32_t)&kernel_end;
}

static void print_usage_bar(uint32_t used, uint32_t capacity)
{
    uint32_t pct = (used * 100) / capacity;
    int filled = (int)(pct / 5);

    if (pct < 50)
        set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    else if (pct < 80)
        set_color(VGA_YELLOW, VGA_BLACK);
    else
        set_color(VGA_LIGHT_RED, VGA_BLACK);

    kprintf("  [");
    for (int i = 0; i < 20; i++)
        kprintf(i < filled ? "#" : ".");
    kprintf("] %u%%\n", pct);
    reset_color();
}


static void walk_and_print(uint32_t ebp, uint32_t stack_top, uint32_t stack_base)
{
    kprintf("  How it got here (each line called the one above it):\n\n");

    if (ebp == 0)
    {
        set_color(VGA_LIGHT_GREEN, VGA_BLACK);
        kprintf("    No call chain to show -- this task isn't in the middle of any\n");
        kprintf("    function calls right now (it's idling at its outermost loop).\n");
        reset_color();
        return;
    }

    if (ebp < stack_base || ebp >= stack_top)
    {
        set_color(VGA_YELLOW, VGA_BLACK);
        kprintf("    Can't walk this: the saved frame pointer (0x%x) doesn't even\n", ebp);
        kprintf("    point inside this task's own stack (0x%x - 0x%x).\n", stack_base, stack_top);
        kprintf("    That alone means this task's stack is corrupted somehow.\n");
        reset_color();
        return;
    }

    for (int i = 0; i < STACKMAP_MAX_FRAMES; i++)
    {
        uint32_t *frame = (uint32_t *)ebp;
        uint32_t ret = frame[1];
        uint32_t next_ebp = frame[0];
        int is_last_real_frame = (next_ebp == 0 || next_ebp <= ebp);

        if (ret != 0 && addr_in_kernel_image(ret))
        {
            set_color(VGA_WHITE, VGA_BLACK);
            kprintf("    %2d. called from 0x%x\n", i, ret);
            reset_color();
        }
        else if (is_last_real_frame)
        {
            set_color(VGA_LIGHT_GREEN, VGA_BLACK);
            kprintf("    %2d. this is where the task was launched -- no earlier caller\n", i);
            reset_color();
        }
        else
        {
            set_color(VGA_LIGHT_RED, VGA_BLACK);
            kprintf("    %2d. return address 0x%x doesn't point into kernel code\n", i, ret);
            kprintf("        --> this task's stack looks corrupted here. Stopping the walk\n");
            kprintf("            so we don't print more garbage as if it were real.\n");
            reset_color();
            return;
        }

        if (is_last_real_frame)
            return;

        if (next_ebp < stack_base || next_ebp >= stack_top)
        {
            set_color(VGA_LIGHT_RED, VGA_BLACK);
            kprintf("        --> the next frame pointer (0x%x) leaves this task's stack.\n", next_ebp);
            kprintf("            Stopping here.\n");
            reset_color();
            return;
        }

        ebp = next_ebp;
    }

    set_color(VGA_YELLOW, VGA_BLACK);
    kprintf("    ... call chain is deeper than %d frames, stopping here.\n", STACKMAP_MAX_FRAMES);
    reset_color();
}

void stackmap_dump(const char *name)
{
    if (!all_tasks)
    {
        set_color(VGA_YELLOW, VGA_BLACK);
        kprintf("No tasks exist yet -- nothing to show.\n");
        reset_color();
        return;
    }

    for (task_t *t = all_tasks; t; t = t->all_next)
    {
        if (strcmp(t->name, name) != 0)
            continue;

        uint32_t base = t->kernel_stack_base;
        uint32_t top = t->kernel_stack;

        if (!base)
        {
            set_color(VGA_YELLOW, VGA_BLACK);
            kprintf("'%s' exists but has no kernel stack tracked -- can't inspect it.\n", name);
            reset_color();
            return;
        }

        uint32_t live_esp;
        uint32_t ebp_to_walk;
        if (t == current_task)
        {
            asm volatile("mov %%esp, %0" : "=r"(live_esp));
            asm volatile("mov %%ebp, %0" : "=r"(ebp_to_walk));
        }
        else
        {
            live_esp = t->context_esp;
            ebp_to_walk = *((uint32_t *)t->context_esp + 3);
        }

        uint32_t used = top - live_esp;
        if (used > 4096)
            used = 4096;

        const char *state_word =
            t == current_task ? "currently running" :
            t->state == TASK_READY ? "ready to run, waiting for its turn" :
            t->state == TASK_BLOCKED ? "parked/idle, not scheduled right now" :
            "in some other state";

        set_color(VGA_CYAN, VGA_BLACK);
        kprintf("\n===============================================\n");
        kprintf(" Task '%s'  (PID %u)\n", t->name, t->pid);
        kprintf("===============================================\n");
        reset_color();

        kprintf(" This task is %s.\n", state_word);
        if (t->is_checkpoint_clone)
        {
            set_color(VGA_LIGHT_MAGENTA, VGA_BLACK);
            kprintf(" It's a clone created by restoring a checkpoint -- it resumed\n");
            kprintf(" execution at the exact point the original was frozen at.\n");
            reset_color();
        }

        kprintf("\n Kernel stack usage: %u of 4096 bytes\n", used);
        print_usage_bar(used, 4096);
        if (used * 100 / 4096 >= 80)
        {
            set_color(VGA_LIGHT_RED, VGA_BLACK);
            kprintf("  This is close to using its whole stack. If it goes over,\n");
            kprintf("  the task will corrupt whatever memory sits right after its\n");
            kprintf("  stack -- usually a crash. Worth investigating deep/unbounded\n");
            kprintf("  recursion or large local variables.\n");
            reset_color();
        }
        kprintf("\n");

        walk_and_print(ebp_to_walk, top, base);
        kprintf("\n");
        return;
    }

    set_color(VGA_RED, VGA_BLACK);
    kprintf("No task named '%s' exists right now.\n", name);
    kprintf("Tip: use 'ps' to see the exact names/PIDs of running tasks.\n");
    reset_color();
}
