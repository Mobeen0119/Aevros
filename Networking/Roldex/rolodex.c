#include "rolodex.h"
#include "../mailroom/directory.h"
#include "../../Lib/kprintf.h"
#include "../../Lib/string.h"
#include "../../kernel/Process/task.h"

static rolodex_entry_t book[ROLODEX_CAPACITY];
static uint32_t contradictions;
static uint8_t our_ip[4];
static int have_our_ip;

static int pending_reply;
static uint8_t pending_requester_mac[6];
static uint8_t pending_requester_ip[4];

static rolodex_entry_t *find_entry(const uint8_t ip[4])
{
    for (int i = 0; i < ROLODEX_CAPACITY; i++)
        if (book[i].in_use && memcmp(book[i].ip, ip, 4) == 0)
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

static void expire_stale_entries(void)
{
    uint32_t now = get_ticks();

    for (int i = 0; i < ROLODEX_CAPACITY; i++)
    {
        if (book[i].in_use && (now - book[i].last_seen) > ROLODEX_ENTRY_TIMEOUT_TICKS)
        {
            kprintf("[Rolodex] forgetting %d.%d.%d.%d, hasn't been heard from in a while\n",
                    book[i].ip[0], book[i].ip[1], book[i].ip[2], book[i].ip[3]);
            book[i].in_use = 0;
            book[i].disputed = 0;
        }
    }
}

void rolodex_set_our_ip(const uint8_t ip[4])
{
    memcpy(our_ip, ip, 4);
    have_our_ip = 1;
}

void rolodex_handle(const uint8_t *payload, uint16_t length)
{

    if (length != ARP_PACKET_SIZE)
    {
        kprintf("[Rolodex] rejected malformed ARP, wrong size (%d, expected 28)\n", length);
        return;
    }
    expire_stale_entries();

    uint16_t opcode = (uint16_t)((payload[6] << 8) | payload[7]);
    const uint8_t *sender_mac = payload + 8;
    const uint8_t *sender_ip = payload + 14;
    const uint8_t *target_ip = payload + 24;

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
            existing->disputed = 1;
        }
        else
            existing->last_seen = get_ticks();
    }
    else
    {
        rolodex_entry_t *slot = find_free_slot();
        if (!slot)
        {
            kprintf("[Rolodex] book is full, not learning %d.%d.%d.%d\n",
                    sender_ip[0], sender_ip[1], sender_ip[2], sender_ip[3]);
        }
        else
        {

            memcpy(slot->ip, sender_ip, 4);
            memcpy(slot->mac, sender_mac, 6);

            slot->last_seen = get_ticks();
            slot->in_use = 1;
            slot->disputed = 0;
            kprintf("[Rolodex] learned %d.%d.%d.%d is at %02x:%02x:%02x:%02x:%02x:%02x\n",
                    sender_ip[0], sender_ip[1], sender_ip[2], sender_ip[3],
                    sender_mac[0], sender_mac[1], sender_mac[2], sender_mac[3], sender_mac[4], sender_mac[5]);
        }
    }

    if (opcode == ARP_REQUEST && have_our_ip && memcmp(target_ip, our_ip, 4) == 0)
    {
        pending_reply = 1;
        memcpy(pending_requester_ip, sender_ip, 4);
        memcpy(pending_requester_mac, sender_mac, 4);
        kprintf("[Rolodex] %d.%d.%d.%d is asking who we are, reply ready\n",
                sender_ip[0], sender_ip[1], sender_ip[2], sender_ip[3]);
    }
}


int rolodex_build_reply(uint8_t out_buf[ARP_PACKET_SIZE], const uint8_t our_mac[6])
{
    if (!pending_reply)
        return 0;

    memset(out_buf, 0, ARP_PACKET_SIZE);

    out_buf[0] = 0x00;
    out_buf[1] = 0x01;
    out_buf[2] = 0x08;
    out_buf[3] = 0x00;
    out_buf[4] = 6;
    out_buf[5] = 4;
    out_buf[6] = 0x00;
    out_buf[7] = ARP_REPLY;

    memcpy(out_buf + 8, our_mac, 6);
    memcpy(out_buf + 14, our_ip, 4);
    memcpy(out_buf + 18, pending_requester_mac, 6);
    memcpy(out_buf + 24, pending_requester_ip, 4);

    pending_reply = 1;
    return 1;
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

uint32_t rolodex_contradiction_count(void)
{
    return contradictions;
}

DIRECTORY_ENTRY(0x0806, rolodex_handle, "Rolodex (ARP)");
