#include "memfreeze.h"
#include "../KallocTracker/kalloc_tracker.h"
#include "../../Process/task.h"
#include "../../../Lib/kprintf.h"
#include "../../../Lib/string.h"

static alloc_record_t *frozen = NULL;
static uint32_t frozen_count = 0;
static uint8_t has_snapshot = 0;

void memfreeze_snap(void)
{
    uint32_t size = tracker_table_size();
    alloc_record_t *table = tracker_get_table();

    if (!frozen)
        frozen = kmalloc_raw(sizeof(alloc_record_t) * size);

    if (!frozen)
    {
        kprintf("memfreeze: failed to allocate snapshot buffer\n");
        return;
    }
    memcpy(frozen, table, sizeof(alloc_record_t) * size);
    frozen_count = size;
    has_snapshot = 1;

    uint32_t live = 0;

    for (uint32_t i = 0; i < size; i++)
        if (frozen[i].alive)
            live++;

    kprintf("memfreeze: snapshot taken ... %u live allocations recorded\n", live);
}

void memfreeze_diff(void)
{
    if (!has_snapshot)
    {
        kprintf("memfreeze: no snapshot yet ... run 'memfreeze snap' first\n");
        return;
    }
    alloc_record_t *now = tracker_get_table();
    uint32_t size = tracker_table_size();

    int new_count = 0, gone_count = 0, ghost_count = 0;

    kprintf("\n  MemFreeze diff:\n\n");

    for (uint32_t i = 0; i < size; i++)
    {

        if (now[i].alive)
        {
            uint8_t existed_before = 0;
            for (uint32_t j = 0; j < frozen_count; j++)
            {
                if (frozen[j].alive && frozen[j].ptr == now[i].ptr)
                {
                    existed_before = 1;
                    break;
                }
            }
            if (!existed_before)
            {
                kprintf("    NEW:    %s:%u   %s   %ub   pid %u\n",
                        now[i].file, now[i].line, now[i].func,
                        now[i].size, now[i].pid);
                new_count++;
            }
        }
    }

    for (uint32_t i = 0; i < size; i++)
    {
        if (now[i].alive)
        {
            uint8_t existed_before = 0;
            for (uint32_t j = 0; j < frozen_count; j++)
            {
                if (frozen[j].alive && frozen[j].ptr == now[i].ptr)
                {
                    existed_before = 1;
                    break;
                }
            }
            if (!existed_before)
            {
                kprintf("    NEW:    %s:%u   %s   %ub   pid %u\n",
                        now[i].file, now[i].line, now[i].func,
                        now[i].size, now[i].pid);
                new_count++;
            }
        }
    }
    for (uint32_t i = 0; i < frozen_count; i++)
    {
        if (!frozen[i].alive)
            continue;

        uint8_t still_alive = 0;
        for (uint32_t j = 0; j < size; j++)
        {
            if (now[j].alive && now[j].ptr == frozen[i].ptr)
            {
                still_alive = 1;
                break;
            }
        }

        if (!still_alive)
        {
            kprintf("    GONE:   %s:%u   %s   %ub   pid %u   (freed correctly)\n",
                    frozen[i].file, frozen[i].line, frozen[i].func,
                    frozen[i].size, frozen[i].pid);
            gone_count++;
            continue;
        }

        uint8_t pid_alive = 0;

        if (ready_queue)
        {
            task_t *t = ready_queue;
            do
            {
                if (t->pid == frozen[i].pid && t->state != TASK_ZOMBIE)
                {
                    pid_alive = 1;
                    break;
                }
                t = t->next;
            } while (t != ready_queue);
        }

        if (!pid_alive)
        {
            kprintf("    GHOST:  %s:%u   %s   %ub   pid %u   (owner is dead)\n",
                    frozen[i].file, frozen[i].line, frozen[i].func,
                    frozen[i].size, frozen[i].pid);
            ghost_count++;
        }
    }
    kprintf("\n  summary: %d new, %d freed correctly, %d ghosts\n\n",
            new_count, gone_count, ghost_count);
}
