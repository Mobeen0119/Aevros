#include "Quarantine.h"

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

static inline uint32_t save_and_disable_irq(void)
{
    uint32_t flags;
    asm volatile("pushf; pop %0; cli" : "=r"(flags));
    return flags;
}

static inline void restore_irq(uint32_t flags)
{
    asm volatile("push %0; popf" :: "r"(flags));
}

void quarantine_check_and_act(void)
{
    if (!ready_queue)
        return;

    uint32_t now = get_ticks();

    uint32_t flags = save_and_disable_irq();
    task_t *t = ready_queue;
    do
    {

        try_quarantine(t, now);
        t = t->next;
    } while (t != ready_queue);
    restore_irq(flags);
}

void quarantine_list(void)
{
    if (!ready_queue)
    {
        kprintf("quarantine: no Tasks running\n");
        return;
    }

    #define QUARANTINE_LIST_MAX 64
    uint32_t pids[QUARANTINE_LIST_MAX];
    char names[QUARANTINE_LIST_MAX][TASK_NAME_LEN];
    int found = 0;

    uint32_t flags = save_and_disable_irq();
    task_t *t = ready_queue;
    do
    {
        if (t->state == TASK_QUARANTINED && found < QUARANTINE_LIST_MAX)
        {
            pids[found] = t->pid;
            strncpy(names[found], t->name, TASK_NAME_LEN);
            found++;
        }
        t = t->next;
    } while (t != ready_queue);
    restore_irq(flags);

    for (int i = 0; i < found; i++)
        kprintf("  pid %-4u %-10s quarantined\n", pids[i], names[i]);

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

    uint32_t released_pid = 0;
    int found = 0;

    uint32_t flags = save_and_disable_irq();
    task_t *t = ready_queue;
    do
    {
        if (strcmp(t->name, name) == 0 && t->state == TASK_QUARANTINED)
        {
            t->state = TASK_READY;
            released_pid = t->pid;
            found = 1;
            break;
        }
        t = t->next;
    } while (t != ready_queue);
    restore_irq(flags);

    if (found)
        kprintf("quarantine: pid %u ..... %s resumed\n", released_pid, name);
    else
        kprintf("quarantine: pid %s not found or not quarantined\n", name);
}
