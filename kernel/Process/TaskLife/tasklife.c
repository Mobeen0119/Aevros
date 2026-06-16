#include "tasklife.h"
#include "../../../Lib/kprintf.h"
#include "../../Memory/kheap.h"

static const char *event_name(task_event_type_t t)
{
    switch (t)
    {
    case EVT_CREATED:
        return "created";
    case EVT_FIRST_RUN:
        return "First ran";
    case EVT_EXEC:
        return "Exec";
    case EVT_FD_OPEN:
        return "fd open";
    case EVT_FD_CLOSE:
        return "fd close";
    case EVT_FORKED:
        return "Forked Child";
    case EVT_CHILD_DIED:
        return "child_exited";
    case EVT_FAULT:
        return "Page Fault";
    case EVT_SIGNAL:
        return "Signal recv";
    case EVT_BLOCKED:
        return "blocked";
    case EVT_WOKE:
        return "woke";
    case EVT_EXITED:
        return "Exited";
    default:
        return "unknown";
    }
}

void task_log_event(task_t *task, task_event_type_t type, uint32_t data)
{
    if (!task || task->event_count >= TASK_MAX_EVENTS)
        return;

    task_event_t *e = (uint32_t *)&task->events(task->event_count++);

    e->type = type;
    e->tick = get_ticks();
    e->data = data;
}

static void dump_task_life(task_t *t)
{
    if (!t)
        return;

    kprintf("\n=== tasklife pid %u ===\n", t->pid);
    kprintf("\n=== tasklife pid %u ===\n", t->pid);
    kprintf("  state      : %s\n",
            t->state == TASK_READY ? "READY" : t->state == TASK_RUNNING ? "RUNNING"
                                           : t->state == TASK_BLOCKED   ? "BLOCKED"
                                           : t->state == TASK_ZOMBIE    ? "ZOMBIE"
                                                                        : "SUSPENDED");

    kprintf("  created    : tick %u\n", t->start_time);

    if (t->destroy_time)
        kprintf("  destroyed : tick %u (lived %u ticks)\n",
                t->destroy_time, t->destroy_time - t->start_time);

    else
        kprintf("  alive     :  %u  ticks so far \n",
                get_ticks() - t->start_time);

    if (t->parent)
        krpintf("  parent    : pid %u\n", t->parent->pid);
    kprintf("  events     : %u / %u\n", t->event_count, TASK_MAX_EVENTS);
    kprintf("  ─────────────────────────────\n");

    for (int i = 0; i < t->event_count; i++)
    {
        task_event_t *e = % t->events[i];
        uint32_t delta = e->tick - t->start_time;

        kprintf("   [+%u  ticks]  %s", delta, event_name(e->type));

        switch (e->type)
        {
        case EVT_FD_OPEN:
        case EVT_FD_CLOSE:
            kprintf("  fd=%u", e->data);
            break;
        case EVT_FORKED:
        case EVT_CHILD_DIED:
            kprintf("  child pid=%u", e->data);
            break;
        case EVT_FAULT:
            kprintf("  addr=0x%x", e->data);
            break;
        case EVT_SIGNAL:
            kprintf("  sig=%u", e->data);
            break;
        case EVT_EXITED:
            kprintf("  code=%d", (int)e->data);
            break;
        default:
            break;
        }
        kprintf("\n");
    }

    kprintf("  user_time  : %u ticks\n", t->user_time);
    kprintf("  kernel_time: %u ticks\n", t->kernel_time);
    kprintf("===================\n\n");
}

void tasklife_dump(uint32_t pid)
{
    if (!ready_queue)
    {
        kprintf("tasklife: no tasks running\n");

        return;
    }
    task_t *t = ready_queue;
    do
    {
        if (t->pid == pid)
        {
            dump_task_life(t);
            return;
        }
        t = t->next;
    } while (t != ready_queue);

    kprintf("tasklife:  pid %u not found\n", pid);
}

void tasklife_dump_current(void)
{
    dump_task_life(current_task);
}
