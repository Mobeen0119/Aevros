#include "stackmap.h"
#include "../task.h"
#include "../../../Lib/kprintf.h"
#include "../../../Lib/string.h"
#include "../../../Include/terminal.h"

#define STACKMAP_MAX_FRAMES 12

extern uint32_t kernel_end;

static void walk_and_print(uint32_t ebp, uint32_t stack_top, uint32_t stack_base)
{
    set_color(VGA_WHITE, VGA_BLACK);
    kprintf("  call chain:\n");

    for (int i = 0; i < STACKMAP_MAX_FRAMES; i++)
    {
        if (ebp < stack_base || ebp >= stack_top)
        {
            if (i == 0)
            {
                set_color(VGA_YELLOW, VGA_BLACK);
                kprintf("    (ebp outside stack bounds ... cannot walk)\n");
            }
            break;
        }
        uint32_t *frame = (uint32_t *)ebp;
        uint32_t ret = frame[1];

        if (!ret)
            break;

        if (ret < 0x100000 || ret > (uint32_t)&kernel_end)
        {
            kprintf("    #%d 0x%x  (outside kernel image -- corrupted frame, stopping)\n", i, ret);
            break;
        }

        kprintf("    #%d 0x%x\n", i, ret);
        uint32_t next_ebp = frame[0];

        if (next_ebp <= ebp)
            break;

        ebp = next_ebp;
    }
    reset_color();
}

void stackmap_dump(const char *name)
{
    if (!all_tasks)
    {
        set_color(VGA_YELLOW, VGA_BLACK);
        kprintf("stackmap: no tasks running\n");
        reset_color();
        return;
    }

    for (task_t *t = all_tasks; t; t = t->all_next)
    {
        if (strcmp(t->name, name) == 0)
        {
            uint32_t base = t->kernel_stack_base;
            uint32_t top = t->kernel_stack;

            if (!base)
            {
                set_color(VGA_YELLOW, VGA_BLACK);
                kprintf("stackmap: pid %s has no tracked kernel stack\n", name);
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

            set_color(VGA_CYAN, VGA_BLACK);
            kprintf("\n pid %u (%s)%s\n", t->pid, t->name,
                    t->is_checkpoint_clone ? " [restored checkpoint clone]" : "");
            reset_color();
            set_color(VGA_WHITE, VGA_BLACK);
            kprintf("  state: %s\n",
                    t == current_task ? "RUNNING" :
                    t->state == TASK_READY ? "READY" :
                    t->state == TASK_BLOCKED ? "BLOCKED" : "OTHER");
            kprintf("  kernel stack: %u / 4096 bytes used (%u%%)\n\n",
                    used, (used * 100) / 4096);
            reset_color();

            walk_and_print(ebp_to_walk, top, base);
            kprintf("\n");
            return;
        }
    }

    set_color(VGA_RED, VGA_BLACK);
    kprintf("stackmap: name %s not found\n", name);
    reset_color();
}
