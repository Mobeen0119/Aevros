#include "guestlist6.h"
#include "../../Lib/string.h"
#include "../../kernel/Process/task.h"
#include "../../Lib/kprintf.h"

typedef struct
{
    uint8_t ip[16];
    guestlist6_verdict_t action;

    uint32_t expires_at_tick;
    int in_use;
} guestlist6_entry_t;

static guestlist6_entry_t entries[GUESTLIST6_CAPACITY];

static guestlist6_entry_t *find_entry(const uint8_t ip[16])
{
    for (int i = 0; i < GUESTLIST6_CAPACITY; i++)
    {
        if (entries[i].in_use && memcmp(entries[i].ip, ip, 16) == 0)
            return &entries[i];
    }
    return 0;
}

int guestlist6_set(const uint8_t ip[16], guestlist6_verdict_t action)
{
    guestlist6_entry_t *existing = find_entry(ip);

    if (existing)
    {
        existing->action = action;
        return 1;
    }

    for (int i = 0; i < GUESTLIST6_CAPACITY; i++)
    {
        if (!entries[i].in_use)
        {
            memcpy(entries[i].ip, ip, 16);
            entries[i].action = action;
            entries[i].in_use = 1;
            return 1;
        }
    }
    return 0;
}

void guestlist6_clear(const uint8_t ip[16])
{
    guestlist6_entry_t *e = find_entry(ip);
    if (e)
        e->in_use = 0;
}

guestlist6_verdict_t guestlist6_check(const uint8_t ip[16])
{
    guestlist6_entry_t *e = find_entry(ip);
    if (!e)
        return GUESTLIST6_NO_RULE;

    return e->action;
}

int guestlist6_set_timed(const uint8_t ip[16], guestlist6_verdict_t action, uint32_t duration_ticks)
{
    if (!guestlist6_set(ip, action))
        return 0;

    guestlist6_entry_t *e = find_entry(ip);

    e->expires_at_tick = get_ticks() + duration_ticks;

    return 1;
}

void guestlist6_tick(void)
{
    uint32_t now = get_ticks();

    for (int i = 0; i < GUESTLIST6_CAPACITY; i++)
    {
        if (!entries[i].in_use || entries[i].expires_at_tick == 0)
            continue;

        if (now >= entries[i].expires_at_tick)
        {
            kprintf("[Guestlist6] a timed entry expired, back to no rule\n");
            entries[i].in_use = 0;
        }
    }
}

uint32_t guestlist6_count(void)
{
    uint32_t n = 0;

    for (int i = 0; i < GUESTLIST6_CAPACITY; i++)
        if (entries[i].in_use)
            n++;

    return n;
}