#include "guestlist.h"
#include "../../Lib/string.h"

typedef struct
{
    uint8_t ip[4];
    guestlist_verdict_t action;
    int in_use;
} guestlist_entry_t;

static guestlist_entry_t entries[GUESTLIST_CAPACITY];

static guestlist_entry_t *find_entry(const uint8_t ip[4])
{
    for (int i = 0; i < GUESTLIST_CAPACITY; i++)
    {
        if (entries[i].in_use && memcmp(entries[i].ip, ip, 4) == 0)
            return &entries[i];
    }
    return 0;
}

int guestlist_set(const uint8_t ip[4], guestlist_verdict_t action)
{
    guestlist_entry_t *existing = find_entry(ip);

    if (existing)
    {
        existing->action = action;
        return 1;
    }

    for (int i = 0; i < GUESTLIST_CAPACITY; i++)
    {
        if (!entries[i].in_use)
        {
            memcpy(entries[i].ip, ip, 4);
            entries[i].action = action;
            entries[i].in_use = 1;
            return 1;
        }
    }
    return 0;
}

void guestlist_clear(const uint8_t ip[4])
{
    guestlist_entry_t *e = find_entry(ip);
    if (e)
        e->in_use = 0;
}

guestlist_verdict_t guestlist_check(const uint8_t ip[4])
{
    guestlist_entry_t *e = find_entry(ip);
    if (!e)
        return GUESTLIST_NO_RULE;

    return e->action;
}

uint32_t guestlist_count(void)
{
    uint32_t n = 0;

    for (int i = 0; i < GUESTLIST_CAPACITY; i++)
        if (entries[i].in_use)
            n++;

    return n;
}
