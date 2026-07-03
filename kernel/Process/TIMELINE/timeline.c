#include "timeline.h"
#include "../task.h"
#include "../TaskLife/tasklife.h"
#include "../../../Lib/kprintf.h"
#include "../../../Lib/string.h"

#define TIMELINE_MAX 4096

typedef struct
{
    uint32_t tick;
    uint32_t pid;
    char name[TASK_NAME_LEN];
    task_event_type_t type;
    uint32_t data;
} timeline_entry_t;

static const char *event_name(task_event_type_t t)
{
    switch (t)
    {
    case EVT_CREATED:
        return "Created";
    case EVT_FIRST_RUN:
        return "First ran";
    case EVT_EXEC:
        return "Exec";
    case EVT_FD_OPEN:
        return "Fd open";
    case EVT_FD_CLOSE:
        return "Fd close";
    case EVT_FORKED:
        return "Forked child";
    case EVT_CHILD_DIED:
        return "Child exited";
    case EVT_FAULT:
        return "Page fault";
    case EVT_SIGNAL:
        return "Signal recv";
    case EVT_BLOCKED:
        return "Blocked";
    case EVT_WOKE:
        return "Woke";
    case EVT_EXITED:
        return "Exited";
    default:
        return "Unknown";
    }
}

void timeline_dump(void)
{
    static timeline_entry_t merged[TIMELINE_MAX];
    uint32_t count = 0;
    uint32_t task_count = 0;

    asm volatile("cli");

    if (!ready_queue)
    {
        asm volatile("sti");
        kprintf("Timeline: No Tasks running\n");
        return;
    }

    task_t *t = ready_queue;
    uint32_t safety = 0;
    do
    {
        if (!t)
            break;

        for (int i = 0; i < t->event_count && count < TIMELINE_MAX ; i++)
        {
            merged[count].tick = t->events[i].tick;
            merged[count].pid = t->pid;

            strncpy(merged[count].name, t->name, TASK_NAME_LEN - 1);
            merged[count].name[TASK_NAME_LEN - 1] = '\0';

            merged[count].type = t->events[i].type;
            merged[count].data = t->events[i].data;
            count++;
        }
        task_count++;
        t = t->next;

    } while (t && t != ready_queue && count < TIMELINE_MAX  && ++safety < 10000);


    if (count == 0)
    {
        kprintf("timeline: no events recorded yet\n");
        return;
    }

    for (uint32_t i = 1; i < count ; i++)
    {
        timeline_entry_t key = merged[i];
        int j = (int)i - 1;
        while (j >= 0 && merged[j].tick > key.tick)
        {
            merged[j + 1] = merged[j];
            j--;
        }
        merged[j + 1] = key;
    }

    kprintf("\n  TimeLine: \n\n");

    for (uint32_t i = 0; i < count; i++)
    {
        timeline_entry_t *e = &merged[i];

        kprintf("  tick %-6u pid %-4u %-10s %-14s",
                e->tick, e->pid, e->name, event_name(e->type));

        switch (e->type)
        {
        case EVT_FD_OPEN:
        case EVT_FD_CLOSE:
            kprintf("fd=%u", e->data);
            break;
        case EVT_FORKED:
            kprintf("-> pid %u", e->data);
            break;
        case EVT_CHILD_DIED:
            kprintf("child pid=%u", e->data);
            break;
        case EVT_FAULT:
            kprintf("addr=0x%x", e->data);
            break;
        case EVT_SIGNAL:
            kprintf("sig=%u", e->data);
            break;
        case EVT_EXITED:
            kprintf("code=%d", (int)e->data);
            break;
        default:
            break;
        }
        kprintf("\n");
    }

    kprintf("\n  %u events across %u task(s)\n\n", count, task_count);
}