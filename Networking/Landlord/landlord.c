#include "landlord.h"
#include "../SwitchBoard/switchboard.h"
#include "../Menu/menu.h"
#include "../LockBox/lockbox.h"
#include "../Roldex/rolodex.h"
#include "../Atlas/atlas.h"
#include "../Lottery/lottery.h"
#include "../../kernel/Process/task.h"
#include "../../Lib/string.h"
#include "../../Lib/kprintf.h"

#define DHCP_FIXED_LEN 240
#define DHCP_MAX_PACKET 320
#define PIT_HZ 100
#define DHCP_RETRY_TICKS (5 * PIT_HZ)
#define DHCP_RENEW_FRACTION_NUM 1
#define DHCP_RENEW_FRACTION_DEN 2
static uint8_t dns_server_ip[4];

static const uint8_t zero_ip[4] = {0, 0, 0, 0};
static const uint8_t broadcast_ip[4] = {255, 255, 255, 255};

static landlord_state_t state = LANDLORD_INIT;
static uint32_t xid;
static uint32_t last_action_tick;

static uint8_t offered_ip[4];
static uint8_t server_ip[4];

static uint8_t subnet_mask[4] = {255, 255, 255, 0};
static uint8_t router_ip[4];
static uint32_t lease_seconds;
static uint32_t lease_start_tick;

static uint32_t new_xid(const uint8_t our_mac[6])
{
    uint8_t mac_hi[4] = {our_mac[0], our_mac[1], our_mac[2], our_mac[3]};

    uint8_t mac_lo[4] = {our_mac[4], our_mac[5], 0, 0};

    return lottery_draw_isn(mac_hi, our_mac[4], mac_lo, LANDLORD_CLIENT_PORT);
}

static void build_header(uint8_t *packet, const uint8_t our_mac[6], const uint8_t claddr[4])
{

    memset(packet, 0, DHCP_FIXED_LEN);

    packet[0] = 1;
    packet[1] = 1;
    packet[2] = 6;
    packet[3] = 0;
    packet[4] = (uint8_t)(xid >> 24);
    packet[5] = (uint8_t)(xid >> 16);
    packet[6] = (uint8_t)(xid >> 8);
    packet[7] = (uint8_t)xid;

    packet[10] = 0x80;

    memcpy(packet + 12, claddr, 4);
    memcpy(packet + 28, our_mac, 6);

    packet[236] = 99;
    packet[237] = 130;

    packet[238] = 83;

    packet[239] = 99;
}

static uint16_t append_discover_options(uint8_t *packet)
{
    uint16_t i = DHCP_FIXED_LEN;

    packet[i++] = 53;
    packet[i++] = 1;
    packet[i++] = 1;
    packet[i++] = 55;

    packet[i++] = 3;
    packet[i++] = 1;
    packet[i++] = 3;

    packet[i++] = 6;
    packet[i++] = 255;

    return i;
}

static uint16_t append_request_options(uint8_t *packet)
{
    uint16_t i = DHCP_FIXED_LEN;

    packet[i++] = 53;
    packet[i++] = 1;
    packet[i++] = 3;
    packet[i++] = 50;
    packet[i++] = 4;

    memcpy(packet + i, offered_ip, 4);
    i += 4;

    packet[i++] = 54;

    memcpy(packet + i, server_ip, 4);
    i += 4;

    packet[i++] = 255;

    return i;
}

static void send_discover(const uint8_t our_mac[6])
{

    uint8_t packet[DHCP_MAX_PACKET];

    xid = new_xid(our_mac);

    build_header(packet, our_mac, zero_ip);

    uint16_t len = append_discover_options(packet);

    switchboard_send_udp(LANDLORD_CLIENT_PORT, broadcast_ip, LANDLORD_SERVER_PORT, our_mac, zero_ip, packet, len);

    state = LANDLORD_DISCOVERING;

    last_action_tick = get_ticks();

    kprintf("[Landlord] send DHCP DISCOVER (xid %u)\n", xid);
}

static void send_request(const uint8_t our_mac[6])
{
    uint8_t packet[DHCP_MAX_PACKET];
    build_header(packet, our_mac, zero_ip);

    uint16_t len = append_request_options(packet);
    switchboard_send_udp(LANDLORD_CLIENT_PORT, broadcast_ip, LANDLORD_SERVER_PORT, our_mac, zero_ip, packet, len);

    state = LANDLORD_REQUESTING;
    last_action_tick = get_ticks();

    kprintf("[Landlord] sent DHCPREQUEST for %d.%d.%d.%d\n", offered_ip[0], offered_ip[1], offered_ip[2], offered_ip[3]);
}

static uint8_t parse_options(const uint8_t *packet, uint16_t len)
{

    if (len < DHCP_FIXED_LEN + 4)
        return 0;

    memcpy(offered_ip, packet + 16, 4);

    uint8_t msg_type = 0;

    uint16_t i = DHCP_FIXED_LEN;

    while (i < len && packet[i] != 255)
    {

        uint8_t opt = packet[i++];

        if (i >= len)
            break;

        uint8_t opt_len = packet[i++];

        if (i + opt_len > len)
            break;

        switch (opt)
        {

        case 53:
            if (opt_len >= 1)
                msg_type = packet[i];
            break;

        case 1:
            if (opt_len >= 4)
                memcpy(subnet_mask, packet + i, 4);
            break;

        case 3:
            if (opt_len >= 4)
                memcpy(router_ip, packet + i, 4);
            break;

        case 6:
            if (opt_len >= 4)
                memcpy(dns_server_ip, packet + i, 4);
            break;
        case 54:
            if (opt_len >= 4)
                memcpy(server_ip, packet + i, 4);
            break;
        case 51:
            if (opt_len >= 4)
                lease_seconds = (uint32_t)((packet[i] << 24) | (packet[i + 1] << 16) | (packet[i + 2] << 8) | packet[i + 3]);
            break;
        default:
            break;
        }
        i = (uint16_t)(i + opt_len);
    }
    return msg_type;
}

static void apply_lease(void)
{

    uint8_t network[4];

    for (int i = 0; i < 4; i++)
        network[i] = (uint8_t)(offered_ip[i] & subnet_mask[i]);

    static const uint8_t on_link[4] = {0, 0, 0, 0};

    rolodex_set_ip(offered_ip);

    atlas_add_route(network, subnet_mask, on_link);

    if (router_ip[0] || router_ip[1] || router_ip[2] || router_ip[3])
        atlas_set_default_gateway(router_ip);

    if (lease_seconds == 0)
        lease_seconds = 3600;

    lease_start_tick = get_ticks();

    state = LANDLORD_BOUND;

    kprintf("[Landlord] bound: %d.%d.%d.%d/%d.%d.%d.%d via %d.%d.%d.%d, lease %u sec\n",
            offered_ip[0], offered_ip[1], offered_ip[2], offered_ip[3],
            subnet_mask[0], subnet_mask[1], subnet_mask[2], subnet_mask[3],
            router_ip[0], router_ip[1], router_ip[2], router_ip[3], lease_seconds);
}

void landlord_start(const uint8_t our_mac[6])
{
    menu_open_port(LANDLORD_CLIENT_PORT, 17);

    uint32_t id;

    lockbox_listen(LANDLORD_CLIENT_PORT, 17, &id);

    send_discover(our_mac);
}

void landlord_tick(const uint8_t our_mac[6])
{
    uint8_t src_ip[4];

    uint16_t src_port;

    uint8_t packet[DHCP_MAX_PACKET];

    uint16_t got = switchboard_recv_udp(LANDLORD_CLIENT_PORT, src_ip, &src_port, packet, sizeof(packet));

    if (got > 0)
    {
        uint8_t msg_type = parse_options(packet, got);

        if (state == LANDLORD_DISCOVERING && msg_type == 2)
        {
            kprintf("[Landlord] got DHCPOFFER: %d.%d.%d.%d from %d.%d.%d.%d\n",
                    offered_ip[0], offered_ip[1], offered_ip[2], offered_ip[3],
                    server_ip[0], server_ip[1], server_ip[2], server_ip[3]);
            send_request(our_mac);
            return;
        }
        if (state == LANDLORD_REQUESTING && msg_type == 5)
        {
            apply_lease();
            return;
        }
        if (state == LANDLORD_REQUESTING && msg_type == 6)
        {
            kprintf("[Landlord] server said no (DHCPNAK), starting over\n");
            send_discover(our_mac);
            return;
        }
    }

    uint32_t now = get_ticks();

    if ((state == LANDLORD_DISCOVERING || state == LANDLORD_REQUESTING) && now - last_action_tick >= DHCP_RETRY_TICKS)
    {
        kprintf("[Landlord] no reply, retrying DHCPDISCOVER\n");
        send_discover(our_mac);

        return;
    }
    if (state == LANDLORD_BOUND)
    {
        uint32_t renew_at = lease_start_tick + (lease_seconds * PIT_HZ * DHCP_RENEW_FRACTION_NUM) / DHCP_RENEW_FRACTION_DEN;
        if (now >= renew_at)
        {
            kprintf("[Landlord] lease is getting old, renewing\n");
            send_discover(our_mac);
        }
    }
}

landlord_state_t landlord_get_state(void)
{
    return state;
}

void landlord_get_dns_server(uint8_t out_ip[4])
{
    memcpy(out_ip, dns_server_ip, 4);
}