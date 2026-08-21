#include "rolodex6.h"
#include "../../Lib/string.h"
#include "../../Lib/kprintf.h"
#include "../../kernel/Process/task.h"

typedef struct
{
    uint8_t ip[16];
    uint8_t mac[6];
    uint32_t last_seen;
    int in_use;

} rolodex6_entry_t;

static rolodex6_entry_t book[ROLODEX6_CAPACITY];
static uint8_t our_ip[16];
static int have_our_ip;

static rolodex6_entry_t *find_entry(const uint8_t ip[16])
{
    for (int i = 0; i < ROLODEX6_CAPACITY; i++)
        if (book[i].in_use && memcmp(book[i].ip, ip, 16) == 0)
            return &book[i];

    return 0;
}

void rolodex6_set_ip(const uint8_t ip[16])
{
    memcpy(our_ip, ip, 16);
    have_our_ip = 1;
}

void rolodex6_get_ip(uint8_t out_ip[16])
{
    memcpy(out_ip, our_ip, 16);
}

int rolodex6_have_ip(void)
{
    return have_our_ip;
}

void rolodex6_learn(const uint8_t ip[16], const uint8_t mac[6])
{

    rolodex6_entry_t *e = find_entry(ip);

    if (!e)
    {
        for (int i = 0; i < ROLODEX6_CAPACITY; i++)
        {
            if (!book[i].in_use)
            {
                e = &book[i];
                break;
            }
        }
    }
    if (!e)
        return; // table full

    memcpy(e->ip, ip, 16);

    memcpy(e->mac, mac, 6);

    e->last_seen = get_ticks();

    e->in_use = 1;
}

int rolodex6_lookup(const uint8_t ip[16], uint8_t our_mac[6])
{
    rolodex6_entry_t *e = find_entry(ip);

    if (!e)
        return 0;

    memcpy(our_mac, e->mac, 6);

    return 1;
}

void rolodex6_tick(void)
{
    uint32_t now = get_ticks();

    for (int i = 0; i < ROLODEX6_CAPACITY; i++)
    {
        if (book[i].in_use && (now - book[i].last_seen) > ROLODEX6_ENTRY_TIMEOUT_TICKS)
        {
            kprintf("[Rolodex6] forgetting a neighbor, hasn't been heard from in a while\n");
            book[i].in_use = 0;
        }
    }
}

uint32_t rolodex6_entry_count(void)
{
    uint32_t n = 0;

    for (int i = 0; i < ROLODEX6_CAPACITY; i++)
        if (book[i].in_use)
            n++;

    return n;
}