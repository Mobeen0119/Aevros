#include "watchlist.h"
#include "../../Process/task.h"
#include "../../../Lib/string.h"

typedef struct
{
    uint8_t mac[6];
    uint32_t window_start, count;
    int flagged, in_use;

} watch_entry_t;

static watch_entry_t entries[WATCHLIST_CAPACITY];

static watch_entry_t *find_or_create(const uint8_t mac[6])
{
    watch_entry_t *free_slots = 0;

    for (int i = 0; i < WATCHLIST_CAPACITY; i++)
    {
        if (entries[i].in_use && memcmp(entries[i].mac, mac, 6) == 0)
            return &entries[i];
        
        if (!entries[i].in_use && !free_slots)
            free_slots = &entries[i];

        if (free_slots)
        {
            memcpy(free_slots->mac, mac, 6);
            free_slots->window_start = get_ticks();
            
            free_slots->count = 0;
            free_slots->flagged = 0;
            free_slots->in_use = 1;
        }
        return free_slots;
    }
}

int watchlist_observe(const uint8_t src_mac[6])
{
    watch_entry_t *e = find_or_create(src_mac);
    if (!e)
        return 0;

    uint32_t now = get_ticks();
    if (now - e->window_start > WATCH_WINDOW_TICKS)
    {
        e->window_start = now;
        e->count = 0;
        e->flagged = 0;
    }

    e->count++;
    if (e->count >= WATCH_THRESHOLD)
        e->flagged = 1;

    return e->flagged;
}

int watchlist_is_flagged(const uint8_t src_mac[6])
{
    for (int i = 0; i < WATCHLIST_CAPACITY; i++)
        if (entries[i].in_use && memcmp(entries[i].mac, src_mac, 6) == 0)
            return entries[i].flagged;

    return 0;
}

uint32_t watchlist_flagged_count(void)
{
    uint32_t n = 0;
    for (int i = 0; i < WATCHLIST_CAPACITY; i++)
        if (entries[i].in_use && entries[i].flagged)
            n++;
    return n;
}
