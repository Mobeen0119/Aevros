#include "foyer6.h"
#include "../Rolodex6/rolodex6.h"
#include "../Echo6/echo6.h"
#include "../../kernel/Process/task.h"
#include "../../Lib/string.h"
#include "../../Lib/kprintf.h"

typedef struct
{
    uint8_t target_ip[16];
    uint8_t frame[FOYER6_MAX_FRAME];
    uint16_t len;
    int8_t in_use;
    uint32_t last_attempt;
    uint32_t retry_count;
} foyer6_entry_t;

static foyer6_entry_t slots[FOYER6_CAPACITY];
static uint32_t flushed_total;
static uint32_t dropped_total;

static void send_neighbor_solicitation(const uint8_t our_mac[6], const uint8_t our_ip[16], const uint8_t target_ip[16])
{
    uint32_t pass_id;
    echo6_dispatch_neighbor_solicitation(target_ip, our_mac, our_ip, &pass_id);
}

int foyer6_queue(const uint8_t target_ip[16], const uint8_t our_mac[6], const uint8_t our_ip[16], const uint8_t *frame, uint16_t len)
{
    if (len == 0 || len > FOYER6_MAX_FRAME)
        return 0;

    uint8_t mac[6];

    if (rolodex6_lookup(target_ip, mac))
    {
        uint8_t out[FOYER6_MAX_FRAME];

        memcpy(out, frame, len);
        memcpy(out, mac, 6);

        uint32_t pass_id;
        if (bailiff_request_pass(out, len, &pass_id) && bailiff_present_pass(pass_id, out, len))
        {
            flushed_total++;
            kprintf("[Foyer6] neighbor already known, sent straight through\n");
        }
        return 1;
    }

    int requested_already = 0;
    int free_slot = -1;

    for (int i = 0; i < FOYER6_CAPACITY; i++)
    {
        if (slots[i].in_use && memcmp(slots[i].target_ip, target_ip, 16) == 0)
            requested_already = 1;

        if (!slots[i].in_use && free_slot < 0)
            free_slot = i;
    }
    if (free_slot < 0)
    {
        kprintf("[Foyer6] waiting area full, dropping packet for an unresolved neighbor\n");
        dropped_total++;
        return 0;
    }

    foyer6_entry_t *e = &slots[free_slot];
    e->len = len;
    memcpy(e->frame, frame, len);
    memcpy(e->target_ip, target_ip, 16);
    e->last_attempt = get_ticks();
    e->retry_count = 0;
    e->in_use = 1;

    kprintf("[Foyer6] holding a frame, no MAC yet for this neighbor\n");

    if (!requested_already)
        send_neighbor_solicitation(our_mac, our_ip, target_ip);

    return 1;
}

void foyer6_tick(const uint8_t our_mac[6], const uint8_t our_ip[16])
{
    uint32_t now = get_ticks();

    for (int i = 0; i < FOYER6_CAPACITY; i++)
    {
        foyer6_entry_t *e = &slots[i];

        if (!e->in_use)
            continue;

        uint8_t mac[6];

        if (rolodex6_lookup(e->target_ip, mac))
        {
            memcpy(e->frame, mac, 6);
            uint32_t pass_id;

            if (bailiff_request_pass(e->frame, e->len, &pass_id) && bailiff_present_pass(pass_id, e->frame, e->len))
            {
                flushed_total++;
                kprintf("[Foyer6] neighbor resolved, releasing the held frame\n");
            }
            e->in_use = 0;
            continue;
        }

        if (now - e->last_attempt < FOYER6_RETRY_INTERVAL_TICK)
            continue;

        if (e->retry_count >= FOYER6_MAX_RETRIES)
        {
            dropped_total++;
            kprintf("[Foyer6] giving up, never got a neighbor advertisement back\n");
            e->in_use = 0;
            continue;
        }

        send_neighbor_solicitation(our_mac, our_ip, e->target_ip);
        e->retry_count++;
        e->last_attempt = now;
    }
}

uint32_t foyer6_flushed_count(void)
{
    return flushed_total;
}

uint32_t foyer6_dropped_count(void)
{
    return dropped_total;
}