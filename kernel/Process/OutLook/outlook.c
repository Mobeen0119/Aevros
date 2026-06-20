#include "outlook.h"
#include "../task.h"
#include "../TaskLife/tasklife.h"
#include "../../Memory/KallocTracker/kalloc_tracker.h"
#include "../../../Include/vfs.h"
#include "../../../Lib/kprintf.h"
#include "../../../Lib/string.h"

#define RISK_FD_RATE_TICKS 200
#define RISK_ALLOC_RATE_TICKS 300
#define RISK_ZOMBIE_AGE 80
#define RISK_STACK_PERCENT 70
#define MAX_RISKS 16
#define MAX_SEEN_SITES 32

typedef enum
{
    RISK_HIGH,
    RISK_MED,
    RISK_LOW
} risk_level_t;

typedef enum
{
    RISK_KIND_FD,
    RISK_KIND_ALLOC,
    RISK_KIND_ZOMBIE,
    RISK_KIND_STACK
} risk_kind_t;

typedef struct
{
    risk_level_t level;
    risk_kind_t kind;

    uint32_t pid;
    char name[TASK_NAME_LEN];
    const char *file;
    uint32_t line;
    uint32_t n1, n2;
} risk_t;

static risk_t risks[MAX_RISKS];
static int risk_count = 0;

static void add_risk(risk_level_t level, risk_kind_t kind, uint32_t pid, const char *name,
                     const char *file, uint32_t line, uint32_t n1, uint32_t n2)
{
    if (risk_count >= MAX_RISKS)
        return;

    risk_t *r = &risks[risk_count++];
    r->level = level;
    r->kind = kind;
    r->pid = pid;
    if (name)
        strncpy(r->name, name, TASK_NAME_LEN);
    r->file = file;
    r->line = line;
    r->n1 = n1;
    r->n2 = n2;
}

static void check_fd_rates(task_t *t, uint32_t now)
{
    int opens = 0, closes = 0;

    for (int i = 0; i < t->event_count; i++)
    {

        uint32_t age = now - t->events[i].tick;
        if (age > RISK_FD_RATE_TICKS)
            continue;

        if (t->events[i].type == EVT_FD_OPEN)
            opens++;
        else if (t->events[i].type == EVT_FD_CLOSE)
            closes++;
    }

    if (opens >= 4 && closes <= opens / 4)
    {
        char what[48], why[96];
        kprintf_to_buf(what, sizeof(what), "pid %u (%s) fd usage", t->pid, t->name);

        kprintf_to_buf(why, sizeof(why),
                       "opened %d fd(s) in last %u ticks, closed only %d -- "
                       "missing sys_close calls likely",
                       opens, RISK_FD_RATE_TICKS, closes);

        add_risk(RISK_HIGH, RISK_KIND_FD, t->pid, t->name,
                 NULL, 0, (uint32_t)opens, (uint32_t)closes);
    }
}

static struct
{
    const char *file;
    uint32_t line;
} seen_sites[MAX_SEEN_SITES];
static int seen_count = 0;

static int already_seen(const char *file, uint32_t line)
{
    for (int i = 0; i < seen_count; i++)
    {
        if (seen_sites[i].line == line && strcmp(seen_sites[i].file, file) == 0)
            return 1;
    }
    return 0;
}

static void mark_seen(const char *file, uint32_t line)
{
    if (seen_count < MAX_SEEN_SITES)
    {
        seen_sites[seen_count].file = file;
        seen_sites[seen_count].line = line;
        seen_count++;
    }
}

static void check_alloc_rates(uint32_t now)
{
    seen_count = 0;

    alloc_record_t *table = tracker_get_table();
    uint32_t size = tracker_table_size();

    for (uint32_t i = 0; i < size; i++)
    {
        if (!table[i].alive)
            continue;
        if (already_seen(table[i].file, table[i].line))
            continue;

        uint32_t count_here = 0;

        for (uint32_t j = 0; j < size; j++)
        {

            if (table[j].alive && table[j].line == table[i].line &&
                strcmp(table[j].file, table[i].file) == 0)
                count_here++;
        }
        mark_seen(table[i].file, table[i].line);

        if (count_here >= 6)
            add_risk(RISK_HIGH, RISK_KIND_ALLOC, table[i].pid, NULL,
                     table[i].file, table[i].line, count_here, 0);
    }
}

static void check_zombies(task_t *t, uint32_t now)
{
    if (t->state != TASK_ZOMBIE)
        return;

    uint32_t age = now - t->destroy_time;
    if (age < RISK_ZOMBIE_AGE)
        return;

    risk_level_t level = (age > RISK_ZOMBIE_AGE * 3) ? RISK_HIGH : RISK_MED;

    add_risk(level, RISK_KIND_ZOMBIE, t->pid, t->name,
             NULL, 0, age, t->parent ? t->parent->pid : 0);
}

static void check_stack_pressure(task_t *t)
{
    if (!t->kernel_stack_base)
        return;

    uint32_t used = t->kernel_stack - t->regs.esp;
    if (used > 4096)
        used = 4096;
    uint32_t pct = (used * 100) / 4096;

    if (pct < RISK_STACK_PERCENT)
        return;

    char what[48], why[96];

    risk_level_t level = (pct > 90) ? RISK_HIGH : RISK_MED;

    add_risk(level, RISK_KIND_STACK, t->pid, t->name, NULL, 0, pct, 0);
}

static const char *level_str(risk_level_t l)
{
    switch (l)
    {
    case RISK_HIGH:
        return "HIGH";
    case RISK_MED:
        return "MED";
    default:
        return "LOW";
    }
}

static int level_rank(risk_level_t l)
{
    switch (l)
    {
    case RISK_HIGH:
        return 0;
    case RISK_MED:
        return 1;
    default:
        return 2;
    }
}

static void print_risk(risk_t *r)
{
    switch (r->kind)
    {
    case RISK_KIND_FD:
        kprintf("  %-4s   pid %-4u %-10s fd usage         opened %u in last %u ticks, closed only %u -- missing sys_close likely\n",
                level_str(r->level), r->pid, r->name,
                r->n1, RISK_FD_RATE_TICKS, r->n2);
        break;

    case RISK_KIND_ALLOC:
        kprintf("  %-4s   %s:%-4u allocations          %u live allocations from this line, none freed -- matching kfree may be missing\n",
                level_str(r->level), r->file, r->line, r->n1);
        break;

    case RISK_KIND_ZOMBIE:
        kprintf("  %-4s   pid %-4u %-10s zombie           dead %u ticks, parent pid %u has not called waitpid\n",
                level_str(r->level), r->pid, r->name, r->n1, r->n2);
        break;

    case RISK_KIND_STACK:
        kprintf("  %-4s   pid %-4u %-10s kernel stack     %u%% of 4096 bytes used -- approaching overflow\n",
                level_str(r->level), r->pid, r->name, r->n1);
        break;
    }
}

void outlook_scan(void)
{
    risk_count = 0;

    uint32_t now = get_ticks();

    if (ready_queue)
    {
        task_t *t = ready_queue;

        do
        {
            check_fd_rates(t, now);
            check_zombies(t, now);
            check_stack_pressure(t);
            t = t->next;
        } while (t != ready_queue);
    }

    check_alloc_rates(now);

    kprintf("\n  OutLook:\n\n");

    if (risk_count == 0)
    {
        kprintf("  (nothing trending toward failure right now)\n\n");
        return;
    }

    for (int i = 0; i < risk_count - 1; i++)
        for (int j = 0; j < risk_count - i - 1; j++)
            if (level_rank(risks[j].level) > level_rank(risks[j + 1].level))
            {
                risk_t tmp = risks[j];
                risks[j] = risks[j + 1];
                risks[j + 1] = tmp;
            }

    for (int i = 0; i < risk_count; i++)
        print_risk(&risks[i]);

    kprintf("\n  Most Likely next Failure: ");
    print_risk(&risks[0]);
    kprintf("\n");
}
