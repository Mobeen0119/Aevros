#include "net_observe.h"
#include "../../kernel/CPU/irq_guard.h"

#define NETOBSERVE_QUEUE_SLOTS 64
#define NETOBSERVE_CAPTURE_BYTES 80 // Ethernet 14 + max ipv4 header 60 + 4 bytes of ports = 78
#define NETOBSERVE_DRAIN_MAX 16

#define ETH_HEADER 14
#define ETHERTYPE_IPV4 0x0800
#define ETHERTYPE_ARP 0x0806
#define ETHERTYPE_IPV6 0x86DD

typedef struct
{
    uint8_t direction;
    uint32_t length;
    uint32_t caplen;
    uint8_t data[NETOBSERVE_CAPTURE_BYTES];
} capture_t;

static capture_t queue[NETOBSERVE_QUEUE_SLOTS];
static uint32_t head = 0;
static uint32_t tail = 0;
static uint32_t count = 0;

static netobserve_stats_t stats;

void netobserve_init(void)
{
    uint32_t f = irq_guard_save();

    head = 0;
    tail = 0;
    count = 0;

    stats.captured = 0;
    stats.dropped = 0;
    stats.recorded = 0;

    stats.malformed = 0;
    stats.unknown = 0;

    stats.last_malformed_reason = 0;

    irq_guard_restore(f);
}

void netobserve_capture(netwatch_direction_t direction, const void *frame, uint32_t length)
{
    if (!frame || length == 0)
        return;

    uint32_t f = irq_guard_save();

    stats.captured++;

    if (count >= NETOBSERVE_QUEUE_SLOTS)
    {
        stats.dropped++;
        irq_guard_restore(f);

        return;
    }

    capture_t *slot = &queue[head];
    uint32_t n = length < NETOBSERVE_CAPTURE_BYTES ? length : NETOBSERVE_CAPTURE_BYTES;

    const uint8_t *src = (const uint8_t *)frame;

    for (uint32_t i = 0; i < n; i++)
        slot->data[i] = src[i];

    slot->direction = (uint8_t)direction;
    slot->length = length;

    slot->caplen = n;

    head = (head + 1) % NETOBSERVE_QUEUE_SLOTS;
    count++;

    irq_guard_restore(f);
}

static uint16_t be16(const uint8_t *p)
{
    return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

static uint32_t be_ipv4(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

static uint32_t hash_ipv6(const uint8_t *p)
{
    uint32_t h = 2166136261u;

    for (int i = 0; i < 16; i++)
    {
        h ^= p[i];
        h *= 16777619u;
    }

    return h;
}

static void reject(const char *reason)
{
    stats.malformed++;
    stats.last_malformed_reason = reason;
}

static void observe_arp(netwatch_direction_t dir, const capture_t *c)
{
    const uint8_t *d = c->data;

    if (c->length < ETH_HEADER + 28 || c->caplen < ETH_HEADER + 28)
    {
        reject("arp: frame shorter than an ARP packet");
        return;
    }

    if (be16(d + 14) != 1 || be16(d + 16) != ETHERTYPE_IPV4 || d[18] != 6 || d[19] != 4)
    {
        reject("arp: not Ethernet/IPv4 ARP");
        return;
    }

    netwatch_arp_event(dir, be_ipv4(d + 28), be_ipv4(d + 38));
    stats.recorded++;
}

static void observe_ipv4(netwatch_direction_t dir, const capture_t *c)
{
    const uint8_t *d = c->data;

    if (c->length < ETH_HEADER + 20 || c->caplen < ETH_HEADER + 20)
    {
        reject("ipv4: frame shorter than a minimal IPv4 header");
        return;
    }

    if ((d[14] >> 4) != 4)
    {
        reject("ipv4: version field is not 4");
        return;
    }

    uint32_t ihl = (uint32_t)(d[14] & 0x0F) * 4;

    if (ihl < 20)
    {
        reject("ipv4: header length field below 20 bytes");
        return;
    }

    if (ETH_HEADER + ihl > c->length || ETH_HEADER + ihl > c->caplen)
    {
        reject("ipv4: header length runs past the frame");
        return;
    }

    uint32_t total = be16(d + 16);

    if (total < ihl || ETH_HEADER + total > c->length)
    {
        reject("ipv4: total length inconsistent with frame");
        return;
    }

    uint8_t proto = d[23];
    uint32_t src = be_ipv4(d + 26);
    uint32_t dst = be_ipv4(d + 30);
    uint32_t fragment_offset = be16(d + 20) & 0x1FFF;
    uint32_t l4 = ETH_HEADER + ihl;
    uint32_t l4_len = total - ihl;

    if (proto == 6 || proto == 17)
    {
        if (fragment_offset != 0)
        {
            netwatch_ipv4_event(dir, c->length, src, dst);
            stats.recorded++;
            return;
        }

        if (l4_len < 4 || l4 + 4 > c->caplen)
        {
            reject("ipv4: TCP/UDP header truncated");
            return;
        }

        uint16_t sport = be16(d + l4);
        uint16_t dport = be16(d + l4 + 2);

        if (proto == 6)
            netwatch_tcp_event(dir, c->length, src, dst, sport, dport);
        else
            netwatch_udp_event(dir, c->length, src, dst, sport, dport);

        stats.recorded++;
        return;
    }

    if (proto == 1)
        netwatch_icmp_event(dir, c->length, src, dst);
    else
        netwatch_ipv4_event(dir, c->length, src, dst);

    stats.recorded++;
}

static void observe_ipv6(netwatch_direction_t dir, const capture_t *c)
{
    const uint8_t *d = c->data;

    if (c->length < ETH_HEADER + 40 || c->caplen < ETH_HEADER + 40)
    {
        reject("ipv6: frame shorter than an IPv6 header");
        return;
    }

    if ((d[14] >> 4) != 6)
    {
        reject("ipv6: version field is not 6");
        return;
    }

    uint32_t payload = be16(d + 18);

    if (ETH_HEADER + 40 + payload > c->length)
    {
        reject("ipv6: payload length runs past the frame");
        return;
    }

    uint8_t next = d[20];
    uint32_t src = hash_ipv6(d + 22);
    uint32_t dst = hash_ipv6(d + 38);
    uint16_t sport = 0;
    uint16_t dport = 0;

    if ((next == 6 || next == 17) && payload >= 4 && ETH_HEADER + 40 + 4 <= c->caplen)
    {
        sport = be16(d + ETH_HEADER + 40);
        dport = be16(d + ETH_HEADER + 42);
    }

    netwatch_ipv6_event(dir, c->length, src, dst, sport, dport);
    stats.recorded++;
}

static void observe(const capture_t *c)
{
    netwatch_direction_t dir = (netwatch_direction_t)c->direction;

    if (c->length < ETH_HEADER || c->caplen < ETH_HEADER)
    {
        reject("ethernet: frame shorter than an Ethernet header");
        return;
    }

    uint16_t ethertype = be16(c->data + 12);

    if (ethertype == ETHERTYPE_ARP)
        observe_arp(dir, c);
    else if (ethertype == ETHERTYPE_IPV4)
        observe_ipv4(dir, c);
    else if (ethertype == ETHERTYPE_IPV6)
        observe_ipv6(dir, c);
    else
        stats.unknown++; // counted, not recorded: event with no addresses would pair with any other such event
}

uint32_t netobserve_drain(void)
{
    uint32_t handled = 0;

    while (handled < NETOBSERVE_DRAIN_MAX)
    {
        uint32_t f = irq_guard_save();

        if (count == 0)
        {
            irq_guard_restore(f);
            break;
        }

        observe(&queue[tail]);

        tail = (tail + 1) % NETOBSERVE_QUEUE_SLOTS;
        count--;

        irq_guard_restore(f);
        handled++;
    }

    return handled;
}

void netobserve_get_stats(netobserve_stats_t *out)
{
    if (!out)
        return;

    uint32_t f = irq_guard_save();
    *out = stats;
    irq_guard_restore(f);
}