#include "quarantine.h"
#include "../TaskLife/tasklife.h"
#include "../../../Lib/kprintf.h"
#include "../../../Lib/string.h"

#define QUARANTINE_FD_THRESHOLD_OPENS 9
#define QUARANTINE_FD_THRESHOLD_CLOSES 1
#define QUARANTINE_WINDOW_TICKS 200

static void try_quarantine(task_t *t, uint32_t now)
{
    if (t->state == TASK_QUARANTINED || t->state == TASK_ZOMBIE)
        return;
    int opens = 0, closes = 0;
    for (int i = 0; i < t->event_count; i++)
    {
        if (now - t->events[i].tick > QUARANTINE_WINDOW_TICKS)
            continue;
        if (t->events[i].type == EVT_FD_OPEN)
            opens++;
        if (t->events[i].type == EVT_FD_CLOSE)
            closes++;
    }

    if (opens >= QUARANTINE_FD_THRESHOLD_OPENS &&
        closes <= QUARANTINE_FD_THRESHOLD_CLOSES)
    {
        t->state = TASK_QUARANTINED;
        task_log_event(t, EVT_QUARANTINED, opens);

        kprintf("\n  [KERNEL] pid %u (%s) auto-quarantined at tick %u\n",
                t->pid, t->name, now);
        kprintf("           reason: fd open rate exceeded threshold "
                "(%d opened, %d closed in %u ticks)\n",
                opens, closes, QUARANTINE_WINDOW_TICKS);
        kprintf("           task frozen, not killed -- state preserved "
                "for inspection\n\n");
    }
}

void quarantine_check_and_act(void)
{
    if (!ready_queue)
        return;

    uint32_t now = get_ticks();

    task_t *t = ready_queue;
    do
    {

        try_quarantine(t, now);
        t = t->next;
    } while (t != ready_queue);
}

void quarantine_list(void)
{
    if (!ready_queue)
    {
        kprintf("quarantine: no Tasks running\n");
        return;
    }

    int found = 0;
    task_t *t = ready_queue;
    do
    {
        if (t->state == TASK_QUARANTINED)
        {
            kprintf("  pid %-4u %-10s quarantined\n", t->pid, t->name);
            found++;
        }
        t = t->next;
    } while (t != ready_queue);

    if (!found)
        kprintf("  (nothing quarantined)\n");
}

void quarantine_release(const char *name)
{
    if (!ready_queue)
    {
        kprintf("quarantine: no tasks running\n");
        return;
    }

    task_t *t = ready_queue;
    do
    {
        if (strcmp(t->name, name) == 0 && t->state == TASK_QUARANTINED)
        {
            t->state = TASK_READY;
            kprintf("quarantine: pid %u ..... %s resumed\n", t->pid, name);
            return;
        }
        t = t->next;
    } while (t != ready_queue);

    kprintf("quarantine: pid %s not found or not quarantined\n", name);
}
