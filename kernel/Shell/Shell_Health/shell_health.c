#include <stdint.h>
#include "shell_health.h"

#include "../../Memory/pmm.h"
#include "../../Memory/KallocTracker/kalloc_tracker.h"
#include "../../Process/task.h"

extern task_t *ready_queue;

static int task_count(void)
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

const char *shell_health(void)
{
    uint32_t free_frames = pmm_free_frames();
    uint32_t leaks = tracker_live_count();
    int tasks = task_count();


    if (free_frames < 16)
        return "CRITICAL";


    if (free_frames < 64)
        return "LOW MEMORY";

    if (leaks > 128)
        return "MEMORY LEAK";

    if (tasks > 32)
        return "HIGH LOAD";


    return "STABLE";
}