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
            records[i].ptr = ptr;
            records[i].size = size;
            records[i].file = file;
            records[i].line = line;
            records[i].pid = current_task ? current_task->pid : 0;
            records[i].alive = 1;
            record_count++;
            return;
        }
    }
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

void tracker_dump(void)
{
    kprintf("\n===MemStory===\n");
    kprintf("live allocations : %u\n\n", record_count);

    uint32_t leaked = 0;
    uint32_t leaked_bytes = 0;

    for (int i = 0; i < TRACKER_MAX_ALLOCS; i++)
    {
        if (!records[i].alive)
            continue;

        uint8_t pid_alive = 0;

        if (current_task)
        {
            task_t *t = ready_queue;
            do
            {
                if (t->pid == records[i].pid && t->state != TASK_ZOMBIE)
                {
                    pid_alive = 1;
                    break;
                }
                t = t->next;
            } while (t != ready_queue);
        }
        else
        {
            pid_alive = 1;
        }
        const char *status = pid_alive ? "ALIVE" : "GHOST";

        kprintf("[%s] %s:%u %s()  size=%u  pid=%u  ptr=%x\n",
                status, records[i].file, records[i].line,
                records[i].func, records[i].size, records[i].pid,
                records[i].ptr);

        if (!pid_alive)
        {
            leaked++;
            leaked_bytes += records[i].size;
        }
    }
    kprintf("\nGHOST allocations : %u\n", leaked);
    kprintf("GHOST bytes        : %u\n", leaked_bytes);
    kprintf("=================\n\n");
}

void tracker_live_count(void)
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

        task_t *t = ready_queue;

        if (!t)
            continue;

        uint8_t found = 0;

        do
        {
            if (t->pid == records[i].pid && t->state != TASK_ZOMBIE)
            {
                found = 1;
                break;
            }
            t = t->next;
        } while (t != ready_queue);

        if (!found)
            total += records[i].size;
    }

    return total;
}
