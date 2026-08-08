#include "foyer.h"
#include "../Roldex/rolodex.h"
#include "../../kernel/Process/task.h"
#include "../../Lib/string.h"
#include "../../Lib/kprintf.h"

typedef struct
{
    uint8_t target_ip[4];
    uint8_t frame[FOYER_MAX_FRAME];
    uint16_t len;
    int8_t in_use;
    uint32_t last_attempt;
    uint32_t retry_count;
} foyer_entry_t;

static foyer_entry_t slots[FOYER_CAPACITY];
static uint32_t flushed_total;
static uint32_t dropped_total;

static void send_arp_request(const uint8_t our_mac[6], const uint8_t target_ip[4])
{
    uint32_t pass_id;

    if (rolodex_dispatch_request(target_ip, our_mac, &pass_id))
        bailiff_present_pass(pass_id, rolodex_last_request_frame(), rolodex_last_request_len());
}

int foyer_queue(const uint8_t target_ip[4], const uint8_t our_mac[6], const uint8_t *frame, uint16_t len)
{
    if (len == 0 || len > FOYER_MAX_FRAME)
        return 0;

    uint8_t mac[6];

    if (rolodex_lookup(target_ip, mac))
    {
        uint8_t out[FOYER_MAX_FRAME];

        memcpy(out, frame, len);
        memcpy(out, mac, 6);

        uint32_t pass_id;
        if (bailiff_present_pass(pass_id, frame, len) && bailiff_request_pass(frame, len, &pass_id))
        {
            flushed_total++;
            kprintf("[Foyer] %d.%d.%d.%d already known, sent straight through\n",
                    target_ip[0], target_ip[1], target_ip[2], target_ip[3]);
        }
        return 1;
    }
    int requested_already = 0;
    int free_slot = -1;

    for (int i = 0; i < FOYER_CAPACITY; i++)
    {
        if (slots[i].in_use && memcmp(slots[i].target_ip, target_ip, 4) == 0)
            requested_already = 1;

        if (!slots[i].in_use && free_slot < 0)
            free_slot = i;
    }
    if (free_slot < 0)
    {
        kprintf("[Foyer] waiting area full, dropping packet for %d.%d.%d.%d\n",
                target_ip[0], target_ip[1], target_ip[2], target_ip[3]);
        dropped_total++;
        return 0;
    }

    foyer_entry_t *e = &slots[free_slot];
    e->len = len;
    memcpy(e->frame, frame, len);
  
    memcpy(e->target_ip, target_ip, 4);
    e->last_attempt = get_ticks();
    e->retry_count = 0;
    e->in_use = 1;

    kprintf("[Foyer] holding a frame for %d.%d.%d.%d, no MAC yet\n",
            target_ip[0], target_ip[1], target_ip[2], target_ip[3]);

    if (!requested_already)
        send_arp_request(our_mac, target_ip);

    return 1;
}

void foyer_tick(const uint8_t our_mac[6])

{
    uint32_t now = get_ticks();

    for (int i = 0; i < FOYER_CAPACITY; i++)
    {
        foyer_entry_t *e = &slots[i];

        if (!e->in_use)
            continue;

        uint8_t mac[6];

        if (rolodex_lookup(e->target_ip, mac))
        {
            memcpy(e->frame, mac, 6);
            uint32_t pass_id;

            if (bailiff_request_pass(e->frame, e->len, &pass_id) && bailiff_present_pass(pass_id, e->frame, e->len))
            {
                flushed_total++;
                kprintf("[Foyer] %d.%d.%d.%d resolved, releasing the held frame\n",
                        e->target_ip[0], e->target_ip[1], e->target_ip[2], e->target_ip[3]);
            }
            e->in_use = 0;
            continue;
        }

        if (now - e->last_attempt < FOYER_RETRY_INTERVAL_TICK)
            continue;

        if (e->retry_count >= FOYER_MAX_RETRIES)
        {
            dropped_total++;
            kprintf("[Foyer] giving up on %d.%d.%d.%d, never got a MAC back\n",
                    e->target_ip[0], e->target_ip[1], e->target_ip[2], e->target_ip[3]);
            e->in_use = 0;
            continue;
        }

        send_arp_request(our_mac, e->target_ip);
        e->retry_count++;
        e->last_attempt = now;
    }
}

uint32_t foyer_flushed_count(void)
{
    return flushed_total;
}

uint32_t foyer_dropped_count(void)
{
    return dropped_total;
}