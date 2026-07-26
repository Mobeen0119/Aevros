#include "curfew.h"
#include "../../kernel/Process/task.h"
#include "../../Lib/string.h"

typedef struct
{
    uint8_t ip[4];
    uint32_t window_start;
    uint32_t count;
    uint32_t last_seen;
    int denied;
    int in_use;
} curfew_entry_t;

static curfew_entry_t entries[CURFEW_CAPACITY];
static uint32_t denied_total;

static curfew_entry_t *find_or_create(const uint8_t ip[4])
{

    curfew_entry_t *free_slot = 0;
    curfew_entry_t *oldest = 0;

    for (int i = 0; i < CURFEW_CAPACITY; i++)
    {
        if (entries[i].in_use && memcmp(entries[i].ip, ip, 4) == 0)
            return &entries[i];

        if (!entries[i].in_use && !free_slot)
            free_slot = &entries[i];

        if (entries[i].in_use && (!oldest || entries[i].last_seen < oldest->last_seen))
            oldest = &entries[i];
    }

    curfew_entry_t *target = free_slot ? free_slot : oldest;

    if (!target)
        return 0;

    memcpy(target->ip, ip, 4);
    target->window_start = get_ticks();
    target->count = 0;
    target->denied = 0;
    target->in_use = 1;

    return target;
}

int curfew_check(const uint8_t src_ip[4])
{
    curfew_entry_t *e = find_or_create(src_ip);

    if (!e)
        return 1;

    uint32_t now = get_ticks();

    e->last_seen = now;

    if (now - e->last_seen > CURFEW_WINDOW_TICKS)
    {
        e->window_start = now;
        e->count = 0;
        e->denied = 0;
    }
    e->count++;
    if (e->count >= CURFEW_THRESHOLD && !e->denied)
    {
        e->denied = 1;
        denied_total++;
    }

    return !e->denied;
}

uint32_t curfew_denied_count(void)
{
    return denied_total;
}