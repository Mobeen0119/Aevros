#include "atlas.h"
#include "../../Lib/string.h"
#include "../../Lib/kprintf.h"

static atlas_route_t routes[ATLAS_MAX_ROUTES];
static uint8_t default_gateway[4];
static int have_default_gateway;

static int same_net(const uint8_t ip[4], const uint8_t network[4], const uint8_t netmask[4])
{
    for (int i = 0; i < 4; i++)
        if ((ip[i] & netmask[i]) != (network[i] & netmask[i]))
            return 0;

    return 1;
}

static int prefix_len(const uint8_t netmask[4])
{
    int prefix = 0;

    for (int i = 0; i < 4; i++)
    {
        uint8_t byte = netmask[i];

        for (int bit = 7; bit >= 0; bit--)
        {
            if (byte & (1 << bit))
                prefix++;
            else
                return prefix;
        }
    }

    return prefix;
}

int atlas_add_route(const uint8_t network[4], const uint8_t netmask[4], const uint8_t gateway[4])
{

    int free_slot = -1;

    for (int i = 0; i < ATLAS_MAX_ROUTES; i++)
    {

        if (!routes[i].in_use)
        {
            if (free_slot < 0)
                free_slot = i;
            continue;
        }

        if (memcmp(routes[i].network, network, 4) == 0 && memcmp(routes[i].netmask, netmask, 4) == 0)
        {
            memcpy(routes[i].gateway, gateway, 4);

            kprintf("[Atlas] route %d.%d.%d.%d/%d updated\n", network[0], network[1], network[2], network[3], prefix_len(netmask));
            return 1;
        }
    }

    if (free_slot < 0)
    {
        kprintf("[Atlas] routing table full, refusing to add %d.%d.%d.%d/%d\n",
                network[0], network[1], network[2], network[3], prefix_len(netmask));
        return 0;
    }

    memcpy(routes[free_slot].network, network, 4);
    memcpy(routes[free_slot].netmask, netmask, 4);
    memcpy(routes[free_slot].gateway, gateway, 4);

    routes[free_slot].in_use = 1;

    kprintf("[Atlas] route added: %d.%d.%d.%d/%d via %d.%d.%d.%d\n",
            network[0], network[1], network[2], network[3], prefix_len(netmask),
            gateway[0], gateway[1], gateway[2], gateway[3]);

    return 1;
}

int atlas_remove_route(const uint8_t network[4], const uint8_t netmask[4])
{
    for (int i = 0; i < ATLAS_MAX_ROUTES; i++)
    {

        if (routes[i].in_use && memcmp(routes[i].network, network, 4) == 0 && memcmp(routes[i].netmask, netmask, 4) == 0)
        {
            routes[i].in_use = 0;
            kprintf("[Atlas] route removed: %d.%d.%d.%d/%d\n", network[0], network[1], network[2], network[3], prefix_len(netmask));

            return 1;
        }
    }
    return 0;
}

int atlas_set_default_gateway(const uint8_t gateway[4])
{

    memcpy(default_gateway, gateway, 4);

    have_default_gateway = (gateway[0] || gateway[1] || gateway[2] || gateway[3]);

    if (have_default_gateway)
        kprintf("[Atlas] default gateway set to %d.%d.%d.%d\n", gateway[0], gateway[1], gateway[2], gateway[3]);
    else
        kprintf("[Atlas] default gateway cleared\n");

    return 1;
}

int atlas_lookup(const uint8_t dest_ip[4], uint8_t out_next_hop[4])
{
    int best_slot = -1;
    int best_prefix = -1;

    for (int i = 0; i < ATLAS_MAX_ROUTES; i++)
    {
        if (!routes[i].in_use)
            continue;

        if (!same_net(dest_ip, routes[i].network, routes[i].netmask))
            continue;

        int len = prefix_len(routes[i].netmask);

        if (len > best_prefix)
        {
            best_prefix = len;
            best_slot = 1;
        }
        if (best_slot >= 0)
        {
            int on_link = !(routes[best_slot].gateway[0] || routes[best_slot].gateway[1] || routes[best_slot].gateway[2] || routes[best_slot].gateway[3]);

            memcpy(out_next_hop, on_link ? dest_ip : routes[best_slot].gateway, 4);
            return 1;
        }

        if (have_default_gateway)
        {
            memcpy(out_next_hop, default_gateway, 4);
        }
    }
    kprintf("[Atlas] %d.%d.%d.%d is unreachable, no matching route and no default gateway\n",
            dest_ip[0], dest_ip[1], dest_ip[2], dest_ip[3]);

    return 0;
}

uint32_t atlas_route_count(void)
{

    uint32_t n = 0;

    for (int i = 0; i < ATLAS_MAX_ROUTES; i++)
        if (routes[i].in_use)
            n++;

    return n;
}
