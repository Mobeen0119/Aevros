#include "landlord6.h"
#include "../SwitchBoard6/switchboard6.h"
#include "../Menu/menu.h"
#include "../LockBox6/lockbox6.h"
#include "../../kernel/Process/task.h"
#include "../../Lib/string.h"
#include "../../Lib/kprintf.h"

#define PIT_HZ 100

#define DHCPV6_MSG_INFORMATION_REQUEST 11
#define DHCPV6_MSG_REPLY 7

#define OPT_CLIENTID 1
#define OPT_SERVERID 2
#define OPT_ORO 6
#define OPT_ELAPSED_TIME 8
#define OPT_DNS_SERVERS 23
#define OPT_DOMAIN_LIST 24
#define OPT_INFO_REFRESH_TIME 32

#define DUID_LL_LEN 10 // 2 (type) + 2 (hw type) + 6 (MAC) 

#define LANDLORD6_MAX_PACKET 256
#define LANDLORD6_DEFAULT_REFRESH_SEC 86400u // RFC 8415 21.23 default 

#define IRT_TICKS (1 * PIT_HZ)
#define MRT_TICKS (120 * PIT_HZ)

static const uint8_t dhcpv6_all_servers[16] =
    {0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 2};

static landlord6_state_t state = LANDLORD6_IDLE;

static uint8_t duid[DUID_LL_LEN];
static uint32_t xid; 
static uint32_t first_send_tick;
static uint32_t next_retry_tick;
static uint32_t retry_interval_ticks;

static uint8_t dns_servers[LANDLORD6_MAX_DNS_SERVERS][16];
static uint32_t dns_server_count;
static uint32_t refresh_at_tick;

static void build_duid(const uint8_t our_mac[6])
{
    duid[0] = 0x00;
    duid[1] = 0x03; 
    duid[2] = 0x00;
    duid[3] = 0x01; //Ethernet
    memcpy(duid + 4, our_mac, 6);
}

static uint32_t new_xid(const uint8_t our_mac[6])
{
    uint32_t mix = get_ticks() ^ 0x9E3779B9u;
    mix = mix * 2654435761u;
    mix ^= ((uint32_t)our_mac[2] << 24) | ((uint32_t)our_mac[3] << 16) |
           ((uint32_t)our_mac[4] << 8) | our_mac[5];
    mix = mix * 2246822519u + 3266489917u;
    return mix & 0x00FFFFFFu;
}

static uint16_t append_option(uint8_t *packet, uint16_t i, uint16_t code,const uint8_t *data, uint16_t len)
{
    packet[i++] = (uint8_t)(code >> 8);
    packet[i++] = (uint8_t)(code & 0xFF);
    packet[i++] = (uint8_t)(len >> 8);
    packet[i++] = (uint8_t)(len & 0xFF);

    if (len > 0)
    {
        memcpy(packet + i, data, len);
        i = (uint16_t)(i + len);
    }
    return i;
}

static void send_information_request(const uint8_t our_mac[6], const uint8_t our_ip[16])
{
    uint8_t packet[LANDLORD6_MAX_PACKET];
    uint16_t i = 0;

    packet[i++] = DHCPV6_MSG_INFORMATION_REQUEST;
    packet[i++] = (uint8_t)(xid >> 16);
    packet[i++] = (uint8_t)(xid >> 8);
    packet[i++] = (uint8_t)xid;

    i = append_option(packet, i, OPT_CLIENTID, duid, DUID_LL_LEN);

    uint8_t oro[4] = {0x00, OPT_DNS_SERVERS, 0x00, OPT_DOMAIN_LIST};
    i = append_option(packet, i, OPT_ORO, oro, sizeof(oro));

    uint32_t now = get_ticks();
    uint32_t elapsed_cs = (first_send_tick == 0) ? 0
                                                  : ((now - first_send_tick) * 100) / PIT_HZ;
    if (elapsed_cs > 0xFFFF)
        elapsed_cs = 0xFFFF;
    uint8_t elapsed[2] = {(uint8_t)(elapsed_cs >> 8), (uint8_t)(elapsed_cs & 0xFF)};
   
    i = append_option(packet, i, OPT_ELAPSED_TIME, elapsed, sizeof(elapsed));

    switchboard6_send_udp(LANDLORD6_CLIENT_PORT, dhcpv6_all_servers, LANDLORD6_SERVER_PORT,
                           our_mac, our_ip, packet, i);

    kprintf("[Landlord6] sent INFORMATION-REQUEST (xid %u)\n", xid);
}

static void parse_reply(const uint8_t *packet, uint16_t len, const uint8_t our_mac[6])
{
    if (len < 4)
        return;

    uint32_t reply_xid = ((uint32_t)packet[1] << 16) | ((uint32_t)packet[2] << 8) | packet[3];
    if (packet[0] != DHCPV6_MSG_REPLY || reply_xid != xid)
        return; 

    dns_server_count = 0;
    uint32_t refresh_sec = LANDLORD6_DEFAULT_REFRESH_SEC;
    int saw_our_client_id = 0;

    uint16_t i = 4;
    while (i + 4 <= len)
    {
        uint16_t code = (uint16_t)((packet[i] << 8) | packet[i + 1]);
        uint16_t opt_len = (uint16_t)((packet[i + 2] << 8) | packet[i + 3]);
        uint16_t data_off = (uint16_t)(i + 4);

        if (data_off + opt_len > len)
            break; 

        switch (code)
        {
        case OPT_CLIENTID:
            if (opt_len == DUID_LL_LEN && memcmp(packet + data_off, duid, DUID_LL_LEN) == 0)
                saw_our_client_id = 1;
            break;

        case OPT_DNS_SERVERS:
        {
            uint16_t n = opt_len / 16;
            for (uint16_t k = 0; k < n && dns_server_count < LANDLORD6_MAX_DNS_SERVERS; k++)
            {
                memcpy(dns_servers[dns_server_count], packet + data_off + (uint16_t)(k * 16), 16);
                dns_server_count++;
            }
            break;
        }

        case OPT_INFO_REFRESH_TIME:
            if (opt_len == 4)
                refresh_sec = ((uint32_t)packet[data_off] << 24) | ((uint32_t)packet[data_off + 1] << 16) |
                              ((uint32_t)packet[data_off + 2] << 8) | packet[data_off + 3];
            break;

        default:
            break;
        }

        i = (uint16_t)(data_off + opt_len);
    }

   
    if (!saw_our_client_id)
    {
        kprintf("[Landlord6] got a REPLY that doesn't carry our client id, ignoring\n");
        return;
    }

    (void)our_mac;

    state = LANDLORD6_BOUND;
    refresh_at_tick = get_ticks() + refresh_sec * PIT_HZ;

    kprintf("[Landlord6] bound: %u DNS server(s), refresh in %u sec\n", dns_server_count, refresh_sec);
}

void landlord6_start(const uint8_t our_mac[6], const uint8_t our_ip[16])
{
    menu_open_port(LANDLORD6_CLIENT_PORT, 17);

    uint32_t id;
    lockbox6_listen(LANDLORD6_CLIENT_PORT, 17, &id);

    build_duid(our_mac);
    xid = new_xid(our_mac);
    first_send_tick = get_ticks();
    retry_interval_ticks = IRT_TICKS;

    send_information_request(our_mac, our_ip);

    state = LANDLORD6_INFO_REQUESTING;
    next_retry_tick = get_ticks() + retry_interval_ticks;
}

void landlord6_tick(const uint8_t our_mac[6], const uint8_t our_ip[16])
{
    uint8_t src_ip[16];
    uint16_t src_port;
    uint8_t packet[LANDLORD6_MAX_PACKET];

    uint16_t got = switchboard6_recv_udp(LANDLORD6_CLIENT_PORT, src_ip, &src_port, packet, sizeof(packet));

    if (got > 0 && state == LANDLORD6_INFO_REQUESTING)
    {
        parse_reply(packet, got, our_mac);
        if (state == LANDLORD6_BOUND)
            return;
    }

    uint32_t now = get_ticks();

    if (state == LANDLORD6_INFO_REQUESTING && now >= next_retry_tick)
    {
        retry_interval_ticks = (retry_interval_ticks * 2 > MRT_TICKS) ? MRT_TICKS : retry_interval_ticks * 2;

        xid = new_xid(our_mac);
        send_information_request(our_mac, our_ip);

        next_retry_tick = now + retry_interval_ticks;
        return;
    }

    if (state == LANDLORD6_BOUND && now >= refresh_at_tick)
    {
        kprintf("[Landlord6] refresh timer elapsed, asking again\n");

        retry_interval_ticks = IRT_TICKS;
        first_send_tick = now;
        xid = new_xid(our_mac);
        send_information_request(our_mac, our_ip);

        state = LANDLORD6_INFO_REQUESTING;
        next_retry_tick = now + retry_interval_ticks;
    }
}

landlord6_state_t landlord6_get_state(void)
{
    return state;
}

uint32_t landlord6_get_dns_servers(uint8_t out_ips[][16])
{
    for (uint32_t i = 0; i < dns_server_count; i++)
        memcpy(out_ips[i], dns_servers[i], 16);

    return dns_server_count;
}
