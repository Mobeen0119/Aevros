#include "atlas6.h"
#include "../../Lib/string.h"
#include "../../Lib/kprintf.h"

static atlas6_route_t routes[ATLAS6_MAX_ROUTES];
static uint8_t default_gateway[16];
static int have_default_gateway;

static int is_zero16(const uint8_t ip[16])
{
    for (int i = 0; i < 16; i++)
        if (ip[i])
            return 0;

    return 1;
}

static int matches_prefix(const uint8_t dest_ip[16], const uint8_t network[16], uint8_t prefix_len)
{
    if (prefix_len > 128)
        return 0;

    uint8_t full_bytes = prefix_len / 8;
    uint8_t rem_bits = prefix_len % 8;

    for (uint8_t i = 0; i < full_bytes; i++)
        if (dest_ip[i] != network[i])
            return 0;

    if (rem_bits > 0)
    {
        uint8_t mask = (uint8_t)(0xFF << (8 - rem_bits));
        if ((dest_ip[full_bytes] & mask) != (network[full_bytes] & mask))
            return 0;
    }

    return 1;
}

int atlas6_add_route(const uint8_t network[16], uint8_t prefix_len, const uint8_t gateway[16])
{
    int free_slot = -1;

    for (int i = 0; i < ATLAS6_MAX_ROUTES; i++)
    {
        if (!routes[i].in_use)
        {
            if (free_slot < 0)
                free_slot = i;
            continue;
        }

        if (routes[i].prefix_len == prefix_len && memcmp(routes[i].network, network, 16) == 0)
        {
            memcpy(routes[i].gateway, gateway, 16);
            kprintf("[Atlas6] route /%d updated\n", prefix_len);
            return 1;
        }
    }

    if (free_slot < 0)
    {
        kprintf("[Atlas6] routing table full, refusing to add /%d\n", prefix_len);
        return 0;
    }

    memcpy(routes[free_slot].network, network, 16);
    routes[free_slot].prefix_len = prefix_len;
    memcpy(routes[free_slot].gateway, gateway, 16);
    routes[free_slot].in_use = 1;

    kprintf("[Atlas6] route added: /%d\n", prefix_len);
    return 1;
}

int atlas6_remove_route(const uint8_t network[16], uint8_t prefix_len)
{
    for (int i = 0; i < ATLAS6_MAX_ROUTES; i++)
    {
        if (routes[i].in_use && routes[i].prefix_len == prefix_len && memcmp(routes[i].network, network, 16) == 0)
        {
            routes[i].in_use = 0;
            kprintf("[Atlas6] route removed: /%d\n", prefix_len);
            return 1;
        }
    }
    return 0;
}

int atlas6_set_default_gateway(const uint8_t gateway[16])
{
    memcpy(default_gateway, gateway, 16);
    have_default_gateway = !is_zero16(gateway);

    if (have_default_gateway)
        kprintf("[Atlas6] default gateway set\n");
    else
        kprintf("[Atlas6] default gateway cleared\n");

    return 1;
}

int atlas6_lookup(const uint8_t dest_ip[16], uint8_t out_next_hop[16])
{
    int best_slot = -1;
    int best_prefix = -1;

    for (int i = 0; i < ATLAS6_MAX_ROUTES; i++)
    {
        if (!routes[i].in_use)
            continue;

        if (!matches_prefix(dest_ip, routes[i].network, routes[i].prefix_len))
            continue;

        if ((int)routes[i].prefix_len > best_prefix)
        {
            best_prefix = routes[i].prefix_len;
            best_slot = i;
        }
    }

    if (best_slot >= 0)
    {
        int on_link = is_zero16(routes[best_slot].gateway);
        memcpy(out_next_hop, on_link ? dest_ip : routes[best_slot].gateway, 16);
        return 1;
    }

    if (have_default_gateway)
    {
        memcpy(out_next_hop, default_gateway, 16);
        return 1;
    }

    kprintf("[Atlas6] destination is unreachable, no matching route and no default gateway\n");
    return 0;
}

uint32_t atlas6_route_count(void)
{
    uint32_t n = 0;
    for (int i = 0; i < ATLAS6_MAX_ROUTES; i++)
        if (routes[i].in_use)
            n++;
    return n;
}
