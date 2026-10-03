#ifndef NETWATCH_H
#define NETWATCH_H

#include <stdint.h>

#define NETWATCH_MAX_EVENTS 128

typedef enum
{
    NETWATCH_PROTO_UNKNOWN = 0,
    NETWATCH_PROTO_ETHERNET,
    NETWATCH_PROTO_ARP,

    NETWATCH_PROTO_IPV4,
    NETWATCH_PROTO_IPV6,
    NETWATCH_PROTO_ICMP,

    NETWATCH_PROTO_UDP,
    NETWATCH_PROTO_TCP
} netwatch_protocol_t;

typedef enum
{
    NETWATCH_RX = 0,
    NETWATCH_TX
} netwatch_direction_t;

typedef struct
{
    uint32_t id;

    netwatch_protocol_t protocol;
    netwatch_direction_t direction;

    uint32_t length;

    uint32_t src_ip;
    uint32_t dst_ip;

    uint16_t src_port;
    uint16_t dst_port;

    uint8_t valid;

    uint64_t tick;
} netwatch_event_t;

typedef struct
{
    uint32_t rx_packets;
    uint32_t tx_packets;

    uint32_t rx_bytes;
    uint32_t tx_bytes;

    uint32_t arp_packets;
    uint32_t ipv4_packets;
    uint32_t ipv6_packets;
    uint32_t icmp_packets;
    uint32_t udp_packets;
    uint32_t tcp_packets;

    uint8_t link_up;
} netwatch_stats_t;

typedef struct
{
    netwatch_stats_t stats;

    netwatch_event_t events[NETWATCH_MAX_EVENTS];

    uint32_t event_count;

} netwatch_snapshot_t;

void netwatch_init(void);

void netwatch_packet_rx(netwatch_protocol_t protocol, uint32_t length, uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port);

void netwatch_packet_tx(netwatch_protocol_t protocol, uint32_t length, uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port);

void netwatch_set_link(uint8_t up);

void netwatch_arp_event(netwatch_direction_t direction, uint32_t src_ip, uint32_t dst_ip);

void netwatch_ipv4_event(netwatch_direction_t direction, uint32_t length, uint32_t src_ip, uint32_t dst_ip);

void netwatch_ipv6_event(netwatch_direction_t direction, uint32_t length, uint32_t src_ip, uint32_t dst_ip);

void netwatch_icmp_event(netwatch_direction_t direction, uint32_t length, uint32_t src_ip, uint32_t dst_ip);

void netwatch_udp_event(netwatch_direction_t direction, uint32_t length, uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port);

void netwatch_tcp_event(netwatch_direction_t direction, uint32_t length, uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port);

void netwatch_get_snapshot(netwatch_snapshot_t *snapshot);

void netwatch_get_stats(netwatch_stats_t *stats);

const netwatch_event_t *netwatch_get_event(uint32_t index);

uint32_t netwatch_get_event_count(void);

void netwatch_get_snapshot(netwatch_snapshot_t *snapshot);

void netwatch_clear_events(void);

#endif