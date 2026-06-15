#include "kalloc_tracker.h"
#include "../../Process/task.h"
#include "../../../Include/screen.h"
#include "../../../Lib/kprintf.h"

static alloc_record_t records[TRACKER_MAX_ALLOCS];
static uint32_t record_count = 0;

void tracker_init(void)
{
    for (int i = 0; i < TRACKER_MAX_ALLOCS; i++)
        records[i].alive = 0;
    record_count = 0;
}

void tracker_record(void *ptr, size_t size, const char *file, uint32_t line, const char *func)
{
    if (!ptr)
        return;

    for (int i = 0; i < TRACKER_MAX_ALLOCS; i++)
    {
        if (!records[i].alive)
        {
            records[i].ptr       = ptr;
            records[i].size      = size;
            records[i].file      = file;
            records[i].line      = line;
            records[i].func      = func;
            records[i].pid       = current_task ? current_task->pid : 0;
            records[i].timestamp = get_ticks();
            records[i].alive     = 1;
            record_count++;
            return;
        }
    }

    /* table full — never silently drop */
    kprintf("[MEMSTORY] WARNING: tracker table full (%u slots), allocation at %x not recorded\n",
            TRACKER_MAX_ALLOCS, ptr);
}

void tracker_remove(void *ptr)
{
    if (!ptr)
        return;

    for (int i = 0; i < TRACKER_MAX_ALLOCS; i++)
    {
        if (records[i].alive && records[i].ptr == ptr)
        {
            records[i].alive = 0;
            record_count--;
            return;
        }
    }
}

static uint8_t pid_is_alive(uint32_t pid)
{
    if (!current_task || !ready_queue)
        return 1;

    task_t *t = ready_queue;
    do {
        if (!t) break;
        if (t->pid == pid && t->state != TASK_ZOMBIE)
            return 1;
        t = t->next;
    } while (t && t != ready_queue);

    return 0;
}

void tracker_dump(void)
{
    uint32_t now     = get_ticks();
    uint32_t leaked  = 0;
    uint32_t leaked_bytes = 0;

    kprintf("\n=== MemStory [%u/%u slots used] ===\n\n", record_count, TRACKER_MAX_ALLOCS);

    for (int i = 0; i < TRACKER_MAX_ALLOCS; i++)
    {
        if (!records[i].alive)
            continue;

        uint8_t alive  = pid_is_alive(records[i].pid);
        uint32_t age   = now - records[i].timestamp;
        const char *status = alive ? "ALIVE" : "GHOST";

        kprintf("[%s] %s:%u %s()  size=%u  pid=%u  ptr=%x  age=%u ticks\n",
                status,
                records[i].file, records[i].line, records[i].func,
                records[i].size, records[i].pid,
                records[i].ptr, age);

        if (!alive)
        {
            leaked++;
            leaked_bytes += records[i].size;
        }
    }

    kprintf("\nGHOST allocations : %u\n", leaked);
    kprintf("GHOST bytes        : %u\n", leaked_bytes);
    kprintf("Live  allocations  : %u\n", record_count - leaked);
    kprintf("=================================\n\n");
}

void tracker_dump_pid(uint32_t pid)
{
    uint32_t now   = get_ticks();
    uint32_t count = 0;
    uint32_t bytes = 0;

    kprintf("\n=== MemStory PID %u ===\n\n", pid);

    for (int i = 0; i < TRACKER_MAX_ALLOCS; i++)
    {
        if (!records[i].alive || records[i].pid != pid)
            continue;

        uint32_t age = now - records[i].timestamp;

        kprintf("%s:%u %s()  size=%u  ptr=%x  age=%u ticks\n",
                records[i].file, records[i].line, records[i].func,
                records[i].size, records[i].ptr, age);

        count++;
        bytes += records[i].size;
    }

    if (count == 0)
        kprintf("  no allocations found for pid %u\n", pid);
    else
        kprintf("\ntotal: %u allocations, %u bytes\n", count, bytes);

    kprintf("======================\n\n");
}

void tracker_dump_ghosts(void)
{
    uint32_t now   = get_ticks();
    uint32_t total = 0;
    uint32_t count = 0;

    kprintf("\n=== MemStory GHOSTS ===\n\n");

    for (int i = 0; i < TRACKER_MAX_ALLOCS; i++)
    {
        if (!records[i].alive)
            continue;

        if (pid_is_alive(records[i].pid))
            continue;

        uint32_t age = now - records[i].timestamp;

        kprintf("%s:%u %s()  size=%u  pid=%u  ptr=%x  age=%u ticks\n",
                records[i].file, records[i].line, records[i].func,
                records[i].size, records[i].pid,
                records[i].ptr, age);

        total += records[i].size;
        count++;
    }

    if (count == 0)
        kprintf("  no ghost allocations — memory clean\n");
    else
        kprintf("\nTotal ghost bytes: %u across %u allocations\n", total, count);

    kprintf("======================\n\n");
}

uint32_t tracker_live_count(void)
{
    return record_count;
}

uint32_t tracker_leaked_bytes(void)
{
    uint32_t total = 0;

    for (int i = 0; i < TRACKER_MAX_ALLOCS; i++)
    {
        if (!records[i].alive)
            continue;

        if (!pid_is_alive(records[i].pid))
            total += records[i].size;
    }

    return total;
}
