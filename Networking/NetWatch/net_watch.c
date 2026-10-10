#include "net_watch.h"
#include "../../kernel/CPU/irq_guard.h"
#include "../../Provenance/provenance.h"

#define NETWATCH_NIC_DRIVER_ID 1

extern uint64_t get_ticks(void);

static netwatch_stats_t stats;
static netwatch_event_t events[NETWATCH_MAX_EVENTS];
static uint32_t event_count = 0;

static uint32_t next_event_id = 1;

static void count_direction(netwatch_direction_t direction, uint32_t length)
{
    if (direction == NETWATCH_RX)
    {
        stats.rx_packets++;

        stats.rx_bytes += length;
    }
    else
    {
        stats.tx_packets++;
        stats.tx_bytes += length;
    }
}

static void record_event(netwatch_direction_t direction, netwatch_protocol_t protocol, uint32_t length, uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port)
{
    if (event_count >= NETWATCH_MAX_EVENTS)
    {
        for (uint32_t i = 1; i < NETWATCH_MAX_EVENTS; i++)
            events[i - 1] = events[i];

        event_count = NETWATCH_MAX_EVENTS - 1;
    }

    events[event_count].id = next_event_id++;
    events[event_count].protocol = protocol;

    events[event_count].direction = direction;
    events[event_count].length = length;
    events[event_count].src_ip = src_ip;
    events[event_count].dst_ip = dst_ip;

    events[event_count].src_port = src_port;

    events[event_count].dst_port = dst_port;
    events[event_count].valid = 1;
    events[event_count].tick = get_ticks();

    uint32_t event_id = events[event_count].id;
    event_count++;

    // driver is origin of every packet event it sees, RX or TX
    provenance_entity_ref_t nic = {PROVENANCE_ENTITY_DRIVER, NETWATCH_NIC_DRIVER_ID};
    provenance_entity_ref_t pkt = {PROVENANCE_ENTITY_NETWORK, event_id};

    provenance_record(nic, pkt, PROVENANCE_RELATION_CREATED,
                      direction == NETWATCH_RX ? PROVENANCE_REASON_DELIVERY : PROVENANCE_REASON_SYSTEM_ACTION);
}

void netwatch_init(void)
{
    stats.rx_packets = 0;
    stats.tx_packets = 0;

    stats.rx_bytes = 0;
    stats.tx_bytes = 0;
    stats.arp_packets = 0;
    stats.ipv4_packets = 0;

    stats.ipv6_packets = 0;

    stats.icmp_packets = 0;

    stats.udp_packets = 0;

    stats.tcp_packets = 0;
    stats.link_up = 0;

    event_count = 0;
    next_event_id = 1;
}

void netwatch_packet_rx(netwatch_protocol_t protocol, uint32_t length, uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port)
{
    stats.rx_packets++;
    stats.rx_bytes += length;

    record_event(NETWATCH_RX, protocol, length, src_ip, dst_ip, src_port, dst_port);
}

void netwatch_packet_tx(netwatch_protocol_t protocol, uint32_t length, uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port)
{
    stats.tx_packets++;
    stats.tx_bytes += length;

    record_event(NETWATCH_TX, protocol, length, src_ip, dst_ip, src_port, dst_port);
}

void netwatch_set_link(uint8_t up)
{
    stats.link_up = up ? 1 : 0;
}

void netwatch_arp_event(netwatch_direction_t direction, uint32_t src_ip, uint32_t dst_ip)
{
    stats.arp_packets++;
    count_direction(direction, 0);

    record_event(direction, NETWATCH_PROTO_ARP, 0, src_ip, dst_ip, 0, 0);
}

void netwatch_ipv4_event(netwatch_direction_t direction, uint32_t length, uint32_t src_ip, uint32_t dst_ip)
{
    stats.ipv4_packets++;
    count_direction(direction, length);

    record_event(direction, NETWATCH_PROTO_IPV4, length, src_ip, dst_ip, 0, 0);
}

void netwatch_ipv6_event(netwatch_direction_t direction, uint32_t length, uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port)
{
    stats.ipv6_packets++;
    count_direction(direction, length);
    record_event(direction, NETWATCH_PROTO_IPV6, length, src_ip, dst_ip, src_port, dst_port);
}

void netwatch_icmp_event(netwatch_direction_t direction, uint32_t length, uint32_t src_ip, uint32_t dst_ip)
{
    stats.icmp_packets++;
    count_direction(direction, length);
    record_event(direction, NETWATCH_PROTO_ICMP, length, src_ip, dst_ip, 0, 0);
}

void netwatch_udp_event(netwatch_direction_t direction, uint32_t length, uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port)
{
    stats.udp_packets++;
    count_direction(direction, length);
    record_event(direction, NETWATCH_PROTO_UDP, length, src_ip, dst_ip, src_port, dst_port);
}

void netwatch_tcp_event(netwatch_direction_t direction, uint32_t length, uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port)
{
    stats.tcp_packets++;
    count_direction(direction, length);

    record_event(direction, NETWATCH_PROTO_TCP, length, src_ip, dst_ip, src_port, dst_port);
}

void netwatch_get_snapshot(netwatch_snapshot_t *snapshot)
{
    if (!snapshot)
        return;

    // interrupts off for copy
    uint32_t f = irq_guard_save();

    snapshot->stats = stats;
    snapshot->event_count = event_count;

    for (uint32_t i = 0; i < event_count; i++)
        snapshot->events[i] = events[i];

    for (uint32_t i = event_count; i < NETWATCH_MAX_EVENTS; i++)
        snapshot->events[i].valid = 0;

    irq_guard_restore(f);
}

void netwatch_get_stats(netwatch_stats_t *output)
{
    if (!output)
        return;

    uint32_t f = irq_guard_save();
    *output = stats;
    irq_guard_restore(f);
}

const netwatch_event_t *netwatch_get_event(uint32_t index)
{

    if (index >= event_count)
        return 0;

    return &events[index];
}

uint32_t netwatch_get_event_count(void)
{
    return event_count;
}

void netwatch_clear_events(void)
{
    uint32_t f = irq_guard_save();

    for (uint32_t i = 0; i < event_count; i++)
        events[i].valid = 0;

    event_count = 0;

    irq_guard_restore(f);
}