#include "stackmap.h"
#include "../task.h"
#include "../../../Lib/kprintf.h"
#include "../../../Lib/string.h"

#define STACKMAP_MAX_FRAMES 12

static void walk_and_print(uint32_t ebp, uint32_t stack_top, uint32_t stack_base)
{
    kprintf("  call chain:\n");

    for (int i = 0; i < STACKMAP_MAX_FRAMES; i++)
    {
        if (ebp < stack_base || ebp >= stack_top)
        {
            if (i == 0)
                kprintf("    (ebp outside stack bounds ... cannot walk\n)");
            break;
        }
        uint32_t *frame = (uint32_t *)ebp;
        uint32_t ret = frame[i];

        if (!ret)
            break;

        kprintf("    #%d 0x%x\n", i, ret);
        uint32_t next_ebp = frame[0];

        if (next_ebp <= ebp)
            break;

        ebp = next_ebp;
    }
}

void stackmap_dump(const char *name)
{
    if (!ready_queue)
    {
        kprintf("stackmap: no tasks running\n");
        return;
    }

    task_t *t = ready_queue;
    do
    {
        if (strcmp(t->name, name) == 0)
        {
            uint32_t base = t->kernel_stack_base;
            uint32_t top = t->kernel_stack;

            if (!base)
            {
                kprintf("stackmap: pid %s has no tracked kernel stack\n", name);
                return;
            }
            
            uint32_t used = top - t->regs.esp;
            if (used > 4096)
                used = 4096;

            kprintf("\n pid %u (%s)\n", t->pid, t->name);
            kprintf("  kernel stack: %u / 4096 bytes used (%u%%)\n\n",
                    used, (used * 100) / 4096);

            uint32_t ebp_to_walk;
            if (t == current_task)
                asm volatile("mov %%ebp, %0" : "=r"(ebp_to_walk));
            else
                ebp_to_walk = t->regs.ebp;

            walk_and_print(ebp_to_walk, top, base);
            kprintf("\n");
            return;
        }
        t = t->next;
    } while (t != ready_queue);

    kprintf("stackmap: name %s not found\n", name);
}
