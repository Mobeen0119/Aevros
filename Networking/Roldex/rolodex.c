#include "rolodex.h"
#include "../mailroom/directory.h"
#include "../../Lib/kprintf.h"
#include "../../Lib/string.h"
#include "../../kernel/Process/task.h"
#include "../Bailiff/bailiff.h"

static rolodex_entry_t book[ROLODEX_CAPACITY];
static uint32_t contradictions;
static uint8_t our_ip[4];
static int have_our_ip;

static uint8_t last_request_frame[6 + 6 + 2 + ARP_PACKET_SIZE];
static uint16_t last_request_len;

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

void rolodex_set_ip(const uint8_t ip[4])
{
    memcpy(our_ip, ip, 4);
    have_our_ip = 1;
}

static void learn_or_dispute(const uint8_t ip[4], const uint8_t mac[6])
{
    expire_stale_entries();

    rolodex_entry_t *existing = find_entry(ip);

    if (existing)
    {
        if (memcmp(existing->mac, mac, 6))
        {
            kprintf("[Rolodex] CONTRADICTION: %d.%d.%d.%d was %02x:%02x:%02x:%02x:%02x:%02x, now claimed by %x:%x:%x:%x:%x:%x\n",
                    ip[0], ip[1], ip[2], ip[3],
                    existing->mac[0], existing->mac[1], existing->mac[2], existing->mac[3], existing->mac[4], existing->mac[5],
                    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            contradictions++;
            existing->disputed = 1;
        }
        else
            existing->last_seen = get_ticks();
        return;
    }

    rolodex_entry_t *slot = find_free_slot();
    if (!slot)
    {
        kprintf("[Rolodex] book is full, not learning %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
        return;
    }

    memcpy(slot->ip, ip, 4);
    memcpy(slot->mac, mac, 6);
    slot->last_seen = get_ticks();
    slot->in_use = 1;
    slot->disputed = 0;
    kprintf("[Rolodex] learned %d.%d.%d.%d is at %02x:%02x:%02x:%02x:%02x:%02x\n",
            ip[0], ip[1], ip[2], ip[3], mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void rolodex_learn(const uint8_t ip[4], const uint8_t mac[6])
{
    learn_or_dispute(ip, mac);
}

void rolodex_handle(const uint8_t *payload, uint16_t length, const uint8_t src_mac[6])
{

    if (length != ARP_PACKET_SIZE)
    {
        kprintf("[Rolodex] rejected malformed ARP, wrong size (%d, expected 28)\n", length);
        return;
    }

    uint16_t opcode = (uint16_t)((payload[6] << 8) | payload[7]);
    const uint8_t *sender_mac = payload + 8;
    const uint8_t *sender_ip = payload + 14;
    const uint8_t *target_ip = payload + 24;

    if (memcmp(sender_mac, src_mac, 6) != 0)
    {
        kprintf("[Rolodex] ARP payload claims sender %02x:%02x:%02x:%02x:%02x:%02x but the frame actually came from %02x:%02x:%02x:%02x:%02x:%02x - refusing to learn from this, ignoring\n",
                sender_mac[0], sender_mac[1], sender_mac[2], sender_mac[3], sender_mac[4], sender_mac[5],
                src_mac[0], src_mac[1], src_mac[2], src_mac[3], src_mac[4], src_mac[5]);
        return;
    }

    learn_or_dispute(sender_ip, sender_mac);

    if (opcode == ARP_REQUEST && have_our_ip && memcmp(target_ip, our_ip, 4) == 0)
    {
        pending_reply = 1;
        memcpy(pending_requester_ip, sender_ip, 4);
        memcpy(pending_requester_mac, sender_mac, 6);
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

    pending_reply = 0;
    return 1;
}

int rolodex_disputed(const uint8_t ip[4])
{
    rolodex_entry_t *e = find_entry(ip);

    if (!e)
        return 0;

    return e->disputed;
}

int rolodex_lookup(const uint8_t ip[4], uint8_t out_mac[6])
{
    rolodex_entry_t *e = find_entry(ip);

    if (!e)
        return 0;

    memcpy(out_mac, e->mac, 6);
    return 1;
}

int rolodex_dispatch_reply(const uint8_t our_mac[6], uint32_t *out_pass_id)
{
    uint8_t arp[ARP_PACKET_SIZE];

    if (!rolodex_build_reply(arp, our_mac))
        return 0;

    static uint8_t frame[6 + 6 + 2 + ARP_PACKET_SIZE];

    memcpy(frame, arp + 18, 6);
    memcpy(frame + 6, our_mac, 6);
    frame[12] = 0x08;
    frame[13] = 0x06;
    memcpy(frame + 14, arp, ARP_PACKET_SIZE);

    return bailiff_request_pass(frame, sizeof(frame), out_pass_id);
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

int rolodex_dispatch_request(const uint8_t target_ip[4], const uint8_t our_mac[6], uint32_t *out_pass_id)
{
    if (!have_our_ip)
        return 0;

    uint8_t arp[ARP_PACKET_SIZE];
    memset(arp, 0, ARP_PACKET_SIZE);

    arp[0] = 0x00;
    arp[1] = 0x01;
    arp[2] = 0x08;
    arp[3] = 0x00;
    arp[4] = 6;
    arp[5] = 4;
    arp[6] = 0x00;
    arp[7] = ARP_REQUEST;

    memcpy(arp + 8, our_mac, 6);
    memcpy(arp + 14, our_ip, 4);
    memset(arp + 18, 0, 6);
    memcpy(arp + 24, target_ip, 4);

    memset(last_request_frame, 0xFF, 6);
    memcpy(last_request_frame + 6, our_mac, 6);
    last_request_frame[12] = 0x08;
    last_request_frame[13] = 0x06;
    memcpy(last_request_frame + 14, arp, ARP_PACKET_SIZE);
    last_request_len = sizeof(last_request_frame);

    return bailiff_request_pass(last_request_frame, last_request_len, out_pass_id);
}

const uint8_t *rolodex_last_request_frame(void) { return last_request_frame; }
uint16_t rolodex_last_request_len(void) { return last_request_len; }

DIRECTORY_ENTRY(0x0806, rolodex_handle, "Rolodex (ARP)");