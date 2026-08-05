#include "sentry.h"
#include "../GuestList/guestlist.h"
#include "../../kernel/Process/task.h"
#include "../../Lib/string.h"
#include "../../Lib/kprintf.h"
#include "../IDS/ids.h"

typedef struct
{
    uint16_t port;
    uint32_t tick;
    int in_use;

} port_hit_t;

typedef struct
{
    uint8_t ip[4];
    port_hit_t hits[SENTRY_PORT_HISTORY];
    int in_use;

} sentry_entry_t;

static sentry_entry_t entries[SENTRY_CAPACITY];
static uint32_t flagged_total;

static sentry_entry_t *find_or_Create(const uint8_t ip[4])
{

    int free_slot = -1;
    for (int i = 0; i < SENTRY_CAPACITY; i++)
    {
        if (entries[i].in_use && memcmp(entries[i].ip, ip, 4) == 0)
            return &entries[i];

        if (!entries[i].in_use && free_slot < 0)
            free_slot = i;
    }
    if (free_slot < 0)
        return 0;

    memcpy(entries[free_slot].ip, ip, 4);
    memset(entries[free_slot].hits, 0, sizeof(entries[free_slot].hits));

    entries[free_slot].in_use = 1;

    return &entries[free_slot];
}

int sentry_observe(const uint8_t src_ip[4], uint16_t dst_port)
{

    sentry_entry_t *e = find_or_Create(src_ip);

    if (!e)
        return 0;

    uint32_t now = get_ticks();

    for (int i = 0; i < SENTRY_PORT_HISTORY; i++)
        if (e->hits[i].in_use && now - e->hits[i].tick >= SENTRY_WINDOW_TICKS)
            e->hits[i].in_use = 0;

    for (int i = 0; i < SENTRY_PORT_HISTORY; i++)
        if (e->hits[i].in_use && e->hits[i].port == dst_port)
        {
            e->hits[i].tick = now;
            return 0;
        }

    int target = -1;

    uint32_t oldest_tick = 0xFFFFFFFF;

    for (int i = 0; i < SENTRY_PORT_HISTORY; i++)
    {
        if (e->hits[i].in_use)
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

    for (int i = 0; i < SENTRY_PORT_HISTORY; i++)
        if (e->hits[i].in_use)
            distinct++;

    if (distinct >= SENTRY_DISTINCT_PORT_THRESHOLD)
    {
        kprintf("[Sentry] %d.%d.%d.%d hit %d distinct ports in the last %d ticks, looks like a port scan - banning\n",
                src_ip[0], src_ip[1], src_ip[2], src_ip[3], distinct, SENTRY_WINDOW_TICKS);
        guestlist_set_timed(src_ip, GUESTLIST_DENIED, SENTRY_BAN_TICKS);

        ids_notify(IDS_EVENT_PORT_SCAN_BAN, src_ip, 0);
        flagged_total++;

        return 1;
    }
    return 0;
}

uint32_t sentry_flagged_count(void)
{
    return flagged_total;
}
