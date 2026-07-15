#include "shell_ui.h"

#include "../../../Lib/kprintf.h"
#include "../../../Lib/string.h"
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

static uint8_t health_color(const char *status)
{
    if (strcmp(status, "STABLE") == 0)
        return VGA_LIGHT_GREEN;
    if (strcmp(status, "LOW MEMORY") == 0 || strcmp(status, "HIGH LOAD") == 0)
        return VGA_YELLOW;
    return VGA_LIGHT_RED; 
}

void shell_draw_prompt(void)
{
    const char *status = shell_health();

    kprintf("\n");
    set_color(VGA_DARK_GREY, VGA_BLACK);
    kprintf("------------------------------------------------------------\n");
    set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    kprintf(" Aevros ");
    set_color(VGA_DARK_GREY, VGA_BLACK);
    kprintf("  tasks ");
    set_color(VGA_WHITE, VGA_BLACK);
    kprintf("%d", count_tasks());
    set_color(VGA_DARK_GREY, VGA_BLACK);
    kprintf("  tick ");
    set_color(VGA_WHITE, VGA_BLACK);
    kprintf("%u", get_ticks());
    set_color(VGA_DARK_GREY, VGA_BLACK);
    kprintf("  free ");
    set_color(VGA_WHITE, VGA_BLACK);
    kprintf("%u", pmm_free_frames());
    set_color(VGA_DARK_GREY, VGA_BLACK);
    kprintf("  leaks ");
    set_color(VGA_WHITE, VGA_BLACK);
    kprintf("%u", tracker_live_count());
    set_color(VGA_DARK_GREY, VGA_BLACK);
    kprintf("  ");
    set_color(health_color(status), VGA_BLACK);
    kprintf("%s\n", status);
    set_color(VGA_DARK_GREY, VGA_BLACK);
    kprintf("------------------------------------------------------------\n");
    set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    kprintf("Aevros");
    set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    kprintf(" > ");
    set_color(VGA_WHITE, VGA_BLACK);
    render();
}