#include "rolodex.h"
#include "../mailroom/directory.h"
#include "../../Lib/kprintf.h"
#include "../../Lib/string.h"
#include "../../kernel/Process/task.h"

static rolodex_entry_t book[ROLODEX_CAPACITY];
static uint32_t contradictions;

static rolodex_entry_t *find_entry(const uint8_t ip[4])
{
    for (int i = 0; i < ROLODEX_CAPACITY; i++)
        if (book[i].in_use && memcmp(book[i].ip, ip, 4)==0)
            return &book[i];
    return 0;
}

static rolodex_entry_t *find_free_slot(void)
{
    for (int i = 0; i < ROLODEX_CAPACITY; i++)
        if (!book[i].in_use)
            return &book[i];
    return 0;
}

void rolodex_handle(const uint8_t *payload, uint16_t length)
{

    if (length != ARP_PACKET_SIZE)
    {
        kprintf("[Rolodex] rejected malformed ARP, wrong size (%d, expected 28)\n", length);
        return;
    }

    const uint8_t *sender_mac = payload + 8;
    const uint8_t *sender_ip = payload + 14;

    rolodex_entry_t *existing = find_entry(sender_ip);

    if (existing)
    {
        if (memcmp(existing->mac, sender_mac, 6))
        {
            kprintf("[Rolodex] CONTRADICTION: %d.%d.%d.%d was %02x:%02x:%02x:%02x:%02x:%02x, now claimed by %x:%x:%x:%x:%x:%x\n",
                    sender_ip[0], sender_ip[1], sender_ip[2], sender_ip[3],
                    existing->mac[0], existing->mac[1], existing->mac[2], existing->mac[3], existing->mac[4], existing->mac[5],
                    sender_mac[0], sender_mac[1], sender_mac[2], sender_mac[3], sender_mac[4], sender_mac[5]);
            contradictions++;
        }
        memcpy(existing->mac, sender_mac, 6);
        existing->last_seen = get_ticks();
        return;
    }

    rolodex_entry_t *slot = find_free_slot();

    if (!slot)
    {
        kprintf("[Rolodex] book is full, not learning %d.%d.%d.%d\n",
                sender_ip[0], sender_ip[1], sender_ip[2], sender_ip[3]);
        return;
    }
    memcpy(slot->ip, sender_ip, 4);
    memcpy(slot->mac, sender_mac, 6);
    slot->last_seen = get_ticks();
    slot->in_use = 1;

    kprintf("[Rolodex] learned %d.%d.%d.%d is at %x:%x:%x:%x:%x:%x\n",
            sender_ip[0], sender_ip[1], sender_ip[2], sender_ip[3],
            sender_mac[0], sender_mac[1], sender_mac[2], sender_mac[3], sender_mac[4], sender_mac[5]);
}

int rolodex_lookup(const uint8_t ip[4], uint8_t out_mac[6])
{
    rolodex_entry_t *e = find_entry(ip);
    if (!e)
        return 0;
    memcpy(out_mac, e->mac, 6);
    return 1;
}

uint32_t rolodex_count(void)
{
    uint32_t n = 0;
    for (int i = 0; i < ROLODEX_CAPACITY; i++)
        if (book[i].in_use)
            n++;
    return n;
}

uint32_t rolodex_contradiction_count(void){
    return contradictions;
}

DIRECTORY_ENTRY(0x0806, rolodex_handle, "Rolodex (ARP)");

