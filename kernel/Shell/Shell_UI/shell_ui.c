#include "shell_ui.h"

#include "../../../Lib/kprintf.h"
#include "../../Process/task.h"
#include "../../Memory/pmm.h"
#include "../../../Include/terminal.h"
#include "../Shell_Health/shell_health.h"
#include "../../Memory/KallocTracker/kalloc_tracker.h"

static int count_tasks(void)
{
    if (!ready_queue)
        return 0;

    int count = 0;
    task_t *t = ready_queue;

    do
    {
        count++;
        t = t->next;
    } while (t != ready_queue);

    return count;
}

void shell_draw_prompt(void)
{
    kprintf("\n");
    set_color(VGA_YELLOW, VGA_BLACK);
    kprintf("============================================================\n");
    kprintf(" Averos | Tasks:%d | Tick:%u | Free:%u | Leaks:%u | %s\n",
            count_tasks(),
            get_ticks(),
            pmm_free_frames(),
            tracker_live_count(),
            shell_health());
    kprintf("============================================================\n");
    kprintf("> ");
    reset_color();
}