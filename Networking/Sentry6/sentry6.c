#include "sentry6.h"
#include "../GuestList6/guestlist6.h"
#include "../../kernel/Process/task.h"
#include "../../Lib/string.h"
#include "../../Lib/kprintf.h"
#include "../IDS6/ids6.h"

typedef struct
{
    uint16_t port;
    uint32_t tick;
    int in_use;
} port_hit_t;

typedef struct
{
    uint8_t ip[16];
    port_hit_t hits[SENTRY6_PORT_HISTORY];
    int in_use;
} sentry6_entry_t;

static sentry6_entry_t entries[SENTRY6_CAPACITY];
static uint32_t flagged_total;

static sentry6_entry_t *find_or_create(const uint8_t ip[16])
{
    int free_slot = -1;

    for (int i = 0; i < SENTRY6_CAPACITY; i++)
    {
        if (entries[i].in_use && memcmp(entries[i].ip, ip, 16) == 0)
            return &entries[i];

        if (!entries[i].in_use && free_slot < 0)
            free_slot = i;
    }
    if (free_slot < 0)
        return 0;

    memcpy(entries[free_slot].ip, ip, 16);
    memset(entries[free_slot].hits, 0, sizeof(entries[free_slot].hits));
    entries[free_slot].in_use = 1;

    return &entries[free_slot];
}

int sentry6_observe(const uint8_t src_ip[16], uint16_t dst_port)
{
    sentry6_entry_t *e = find_or_create(src_ip);
    if (!e)
        return 0;

    uint32_t now = get_ticks();

    for (int i = 0; i < SENTRY6_PORT_HISTORY; i++)
        if (e->hits[i].in_use && now - e->hits[i].tick >= SENTRY6_WINDOW_TICKS)
            e->hits[i].in_use = 0;

    for (int i = 0; i < SENTRY6_PORT_HISTORY; i++)
        if (e->hits[i].in_use && e->hits[i].port == dst_port)
        {
            e->hits[i].tick = now;
            return 0;
        }

    int target = -1;
    uint32_t oldest_tick = 0xFFFFFFFF;

    for (int i = 0; i < SENTRY6_PORT_HISTORY; i++)
    {
        if (!e->hits[i].in_use)
        {
            target = i;
            break;
        }
        if (e->hits[i].tick < oldest_tick)
        {
            oldest_tick = e->hits[i].tick;
            target = i;
        }
    }

    e->hits[target].port = dst_port;
    e->hits[target].tick = now;
    e->hits[target].in_use = 1;

    int distinct = 0;
    for (int i = 0; i < SENTRY6_PORT_HISTORY; i++)
        if (e->hits[i].in_use)
            distinct++;

    if (distinct >= SENTRY6_DISTINCT_PORT_THRESHOLD)
    {
        kprintf("[Sentry6] source hit %d distinct ports in %d ticks, looks like a port scan - banning\n", distinct, SENTRY6_WINDOW_TICKS);
        guestlist6_set_timed(src_ip, GUESTLIST6_DENIED, SENTRY6_BAN_TICKS);
        ids6_notify(IDS6_EVENT_PORT_SCAN_BAN, src_ip, 0);
        flagged_total++;
        return 1;
    }
    return 0;
}

uint32_t sentry6_flagged_count(void)
{
    return flagged_total;
}