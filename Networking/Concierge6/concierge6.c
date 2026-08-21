#include "concierge6.h"
#include "../Echo6/echo6.h"
#include "../Rolodex6/rolodex6.h"
#include "../Atlas6/atlas6.h"
#include "../Landlord6/landlord6.h"
#include "../../kernel/Process/task.h"
#include "../../Lib/string.h"
#include "../../Lib/kprintf.h"

#define PIT_HZ 100
#define RS_RETRY_TICKS (4 * PIT_HZ)  
#define RS_MAX_ATTEMPTS 3           

static concierge6_state_t state = CONCIERGE6_SOLICITING_ROUTER;
static uint8_t link_local_ip[16];
static uint8_t global_ip[16];
static uint8_t active_ip[16]; 
static uint32_t next_rs_tick;
static uint32_t rs_attempts;

static void form_link_local(uint8_t out[16], const uint8_t mac[6])
{
    memset(out, 0, 16);
    out[0] = 0xfe;
    out[1] = 0x80;

    out[8] = mac[0] ^ 0x02;
    out[9] = mac[1];
    out[10] = mac[2];
    out[11] = 0xff;

    out[12] = 0xfe;
    out[13] = mac[3];
    out[14] = mac[4];

    out[15] = mac[5];
}

static void derive_privacy_suffix(uint8_t out8[8], const uint8_t mac[6], const uint8_t prefix[16])
{
    uint32_t h1 = 0x9E3779B9u;
    uint32_t h2 = 0x85EBCA6Bu;

    for (int i = 0; i < 6; i++)
    {
        h1 = (h1 ^ mac[i]) * 2654435761u;
        h2 = (h2 ^ mac[i]) * 2246822519u;
    }
    for (int i = 0; i < 8; i++) // only network bits of a /64 matter 
    {
        h1 = (h1 ^ prefix[i]) * 2654435761u;
        h2 = (h2 ^ prefix[i]) * 2246822519u;
    }
    h1 ^= h1 >> 15;
    h2 ^= h2 >> 13;

    out8[0] = (uint8_t)(h1 >> 24);
    out8[1] = (uint8_t)(h1 >> 16);
    out8[2] = (uint8_t)(h1 >> 8);
    out8[3] = (uint8_t)h1;
    out8[4] = (uint8_t)(h2 >> 24);
    out8[5] = (uint8_t)(h2 >> 16);
    out8[6] = (uint8_t)(h2 >> 8);
    out8[7] = (uint8_t)h2;
}

void concierge6_start(const uint8_t our_mac[6])
{
    form_link_local(link_local_ip, our_mac);
    rolodex6_set_ip(link_local_ip);
    memcpy(active_ip, link_local_ip, 16);

    kprintf("[Concierge6] link-local address ready, soliciting a router\n");

    uint32_t pass_id;
    echo6_dispatch_router_solicitation(our_mac, link_local_ip, &pass_id);

    rs_attempts = 1;
   
    next_rs_tick = get_ticks() + RS_RETRY_TICKS;
    state = CONCIERGE6_SOLICITING_ROUTER;
}

void concierge6_tick(const uint8_t our_mac[6])
{
    if (state == CONCIERGE6_SOLICITING_ROUTER)
    {
        if (echo6_have_router_advertisement())
        {
            uint8_t prefix[16];
            uint8_t prefix_len;
            echo6_get_prefix(prefix, &prefix_len);

            if (prefix_len != 64)
            {
    
                kprintf("[Concierge6] router announced a /%d prefix, not /64 .... SLAAC doesn't apply, staying link-local only\n", prefix_len);
               
                landlord6_start(our_mac, active_ip);
                state = CONCIERGE6_READY;
                return;
            }

            memcpy(global_ip, prefix, 8);

            uint8_t suffix[8];
            derive_privacy_suffix(suffix, our_mac, prefix);

            memcpy(global_ip + 8, suffix, 8);

            rolodex6_set_ip(global_ip);
            memcpy(active_ip, global_ip, 16);

            uint8_t router_ip[16];
            echo6_get_router(router_ip);
            atlas6_set_default_gateway(router_ip);

            static const uint8_t no_gateway[16] = {0};
            atlas6_add_route(prefix, 64, no_gateway); // the prefix itself is on-link 

            kprintf("[Concierge6] SLAAC complete, global address configured\n");

            landlord6_start(our_mac, active_ip);
            state = CONCIERGE6_READY;
            return;
        }

        uint32_t now = get_ticks();
        if (now >= next_rs_tick)
        {
            if (rs_attempts < RS_MAX_ATTEMPTS)
            {
                uint32_t pass_id;
                echo6_dispatch_router_solicitation(our_mac, link_local_ip, &pass_id);
                rs_attempts++;
                next_rs_tick = now + RS_RETRY_TICKS;
                kprintf("[Concierge6] no Router Advertisement yet, retry %u/%u\n", rs_attempts, RS_MAX_ATTEMPTS);
            }
        }
        return;
    }

    landlord6_tick(our_mac, active_ip);
}

concierge6_state_t concierge6_get_state(void)
{
    return state;
}

void concierge6_get_global_ip(uint8_t out_ip[16])
{
    memcpy(out_ip, global_ip, 16);
}
