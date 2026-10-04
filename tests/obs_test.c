#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "Networking/NetWatch/net_observe.h"
#include "Networking/NetTrace/net_trace.h"

static uint64_t now_tick = 0;
uint64_t get_ticks(void) { return now_tick; }

static int fails = 0;
static void check(const char *n, int ok)
{
    printf("%s: %s\n", ok ? "PASS" : "FAIL", n);
    if (!ok)
        fails++;
}

static uint8_t buf[2000];

static void put16(uint8_t *p, uint16_t v)
{
    p[0] = v >> 8;
    p[1] = v & 0xFF;
}
static void eth(uint16_t type)
{
    memset(buf, 0, sizeof buf);
    memset(buf, 0xAA, 6);
    memset(buf + 6, 0xBB, 6);
    put16(buf + 12, type);
}

static uint32_t ipv4(uint8_t proto, const uint8_t s[4], const uint8_t d[4], uint8_t ihl_words, uint16_t sport, uint16_t dport, uint16_t frag_field)
{
    eth(0x0800);
    uint32_t ihl = ihl_words * 4u;
    buf[14] = 0x40 | ihl_words;

    uint16_t total = (uint16_t)(ihl + 8);
    put16(buf + 16, total);

    put16(buf + 20, frag_field);
    buf[22] = 64;

    buf[23] = proto;
    memcpy(buf + 26, s, 4);

    memcpy(buf + 30, d, 4);

    put16(buf + 14 + ihl, sport);

    put16(buf + 14 + ihl + 2, dport);
    uint32_t len = 14 + total;
    return len < 60 ? 60 : len;
}

static uint32_t arp(const uint8_t sip[4], const uint8_t tip[4], uint8_t hlen)
{
    eth(0x0806);
    put16(buf + 14, 1);
    put16(buf + 16, 0x0800);

    buf[18] = hlen;
    buf[19] = 4;
    put16(buf + 20, 1);

    memcpy(buf + 28, sip, 4);
    memcpy(buf + 38, tip, 4);

    return 60;
}

static uint32_t ipv6(uint8_t next, uint8_t sfirst, uint8_t dfirst, uint16_t sport, uint16_t dport, uint16_t payload_len)
{
    eth(0x86DD);
    buf[14] = 0x60;

    put16(buf + 18, payload_len);
    buf[20] = next;
    buf[21] = 64;

    buf[22] = sfirst;
    buf[38] = dfirst;

    put16(buf + 54, sport);
    put16(buf + 56, dport);

    uint32_t len = 14 + 40 + 8;

    return len < 60 ? 60 : len;
}

static void cap(netwatch_direction_t d, uint32_t len)
{
    netobserve_capture(d, buf, len);
}

static void drain_all(void)
{
    while (netobserve_drain())
    {
    }
}

static void reset(void)
{
    netwatch_init();

    netobserve_init();
}

static netobserve_stats_t st(void)
{
    netobserve_stats_t s;
    netobserve_get_stats(&s);
    return s;
}

static const netwatch_event_t *ev(uint32_t i)
{
    return netwatch_get_event(i);
}

static const uint8_t ME[4] = {10, 0, 2, 15};
static const uint8_t GW[4] = {10, 0, 2, 2};

#define ME_PACKED 0x0A00020Fu
#define GW_PACKED 0x0A000202u

int main(void)
{
    // TCP request then reply
    reset();
    now_tick = 100;
    cap(NETWATCH_TX, ipv4(6, ME, GW, 5, 49152, 80, 0));
    now_tick = 110;
    cap(NETWATCH_RX, ipv4(6, GW, ME, 5, 80, 49152, 0));
    drain_all();

    check("tcp: two events recorded", netwatch_get_event_count() == 2);

    check("tcp: request fields", ev(0) && ev(0)->protocol == NETWATCH_PROTO_TCP && ev(0)->direction == NETWATCH_TX &&
                                     ev(0)->src_ip == ME_PACKED && ev(0)->dst_ip == GW_PACKED && ev(0)->src_port == 49152 && ev(0)->dst_port == 80);
    check("tcp: reply fields", ev(1) && ev(1)->direction == NETWATCH_RX && ev(1)->src_ip == GW_PACKED && ev(1)->src_port == 80);

    netwatch_stats_t ns;
    netwatch_get_stats(&ns);

    check("tcp: rx/tx totals and bytes counted", ns.tx_packets == 1 && ns.rx_packets == 1 && ns.tx_bytes == 60 && ns.rx_bytes == 60 && ns.tcp_packets == 2);

    netwatch_snapshot_t snap;
    netwatch_get_snapshot(&snap);
    nettrace_init();

    nettrace_rebuild_at(&snap, 120);
    check("tcp: nettrace pairs the real frames end to end",
          nettrace_get_event(1)->relation == NETTRACE_RELATION_RESPONSE && nettrace_get_event(0)->status == NETTRACE_STATUS_ANSWERED);

    // UDP, ICMP, other IPv4
    reset();

    cap(NETWATCH_TX, ipv4(17, ME, GW, 5, 5353, 53, 0));
    cap(NETWATCH_TX, ipv4(1, ME, GW, 5, 0, 0, 0));
    cap(NETWATCH_RX, ipv4(2, GW, ME, 5, 0, 0, 0));

    drain_all();
    check("udp event with ports", ev(0) && ev(0)->protocol == NETWATCH_PROTO_UDP && ev(0)->src_port == 5353 && ev(0)->dst_port == 53);

    check("icmp event, no ports", ev(1) && ev(1)->protocol == NETWATCH_PROTO_ICMP && ev(1)->src_port == 0);
    check("other ipv4 protocol recorded as IPV4", ev(2) && ev(2)->protocol == NETWATCH_PROTO_IPV4);

    reset();

    cap(NETWATCH_TX, ipv4(6, ME, GW, 15, 1234, 4321, 0));
    drain_all();
    check("ihl=15: ports read past the options", ev(0) && ev(0)->src_port == 1234 && ev(0)->dst_port == 4321);

    // non-first fragment has no L4 header
    reset();

    cap(NETWATCH_RX, ipv4(6, GW, ME, 5, 0xDEAD, 0xBEEF, 0x00B9));
    drain_all();
    check("non-first fragment: recorded without ports", ev(0) && ev(0)->protocol == NETWATCH_PROTO_IPV4 && ev(0)->src_port == 0);

    // ARP request and reply pair up
    reset();
    cap(NETWATCH_TX, arp(ME, GW, 6));

    cap(NETWATCH_RX, arp(GW, ME, 6));

    drain_all();
    check("arp: both recorded with sender/target", ev(0) && ev(1) && ev(0)->src_ip == ME_PACKED && ev(0)->dst_ip == GW_PACKED && ev(1)->src_ip == GW_PACKED);
    netwatch_get_snapshot(&snap);

    nettrace_init();
    nettrace_rebuild_at(&snap, 10);
    check("arp: request/reply pair in nettrace", nettrace_get_event(1)->relation == NETTRACE_RELATION_RESPONSE);

    // IPv6: ports read, reverse direction hashes match
    reset();

    cap(NETWATCH_TX, ipv6(6, 0xFE, 0x20, 40000, 443, 8));
    cap(NETWATCH_RX, ipv6(6, 0x20, 0xFE, 443, 40000, 8));

    drain_all();
    check("ipv6: ports recorded", ev(0) && ev(0)->protocol == NETWATCH_PROTO_IPV6 && ev(0)->src_port == 40000 && ev(0)->dst_port == 443);
    check("ipv6: reverse addresses hash to matching values", ev(0) && ev(1) && ev(0)->src_ip == ev(1)->dst_ip && ev(0)->dst_ip == ev(1)->src_ip && ev(0)->src_ip != ev(0)->dst_ip);

    netwatch_get_snapshot(&snap);
    nettrace_init();
    nettrace_rebuild_at(&snap, 10);
    check("ipv6: pair in nettrace", nettrace_get_event(1)->relation == NETTRACE_RELATION_RESPONSE);

    // malformed frames record nothing, count

    reset();

    eth(0x0800);

    cap(NETWATCH_RX, 10);
    {
        uint32_t l = ipv4(6, GW, ME, 5, 1, 2, 0);
        buf[14] = 0x44;

        cap(NETWATCH_RX, l);
    }
    {
        uint32_t l = ipv4(6, GW, ME, 5, 1, 2, 0);
        buf[14] = 0x55;
        cap(NETWATCH_RX, l);
    }
    {

        uint32_t l = ipv4(6, GW, ME, 5, 1, 2, 0);
        put16(buf + 16, 1400);

        cap(NETWATCH_RX, l);
    }
    {
        uint32_t l = ipv4(6, GW, ME, 5, 1, 2, 0);
        put16(buf + 16, 10);
        cap(NETWATCH_RX, l);
    }
    {
        uint32_t l = ipv4(6, GW, ME, 15, 1, 2, 0);
        cap(NETWATCH_RX, 14 + 30);
        (void)l;
    }
    {
        uint32_t l = arp(GW, ME, 8);
        cap(NETWATCH_RX, l);
    }
    {
        uint32_t l = arp(GW, ME, 6);

        cap(NETWATCH_RX, 30);
        (void)l;
    }
    {
        uint32_t l = ipv6(6, 1, 2, 1, 2, 9000);
        cap(NETWATCH_RX, l);
    }
    {
        uint32_t l = ipv6(6, 1, 2, 1, 2, 8);
        buf[14] = 0x40;

        cap(NETWATCH_RX, l);
    }
    drain_all();
    netobserve_stats_t s = st();
    check("malformed: nothing recorded", netwatch_get_event_count() == 0);
    printf("  (malformed counted: %u)\n", s.malformed);
    check("malformed: all ten counted", s.malformed == 10);
    check("malformed: reason is kept", s.last_malformed_reason != 0 && strlen(s.last_malformed_reason) > 0);

    // unknown ethertype: counted, not recorded
    reset();
    eth(0x88CC);
    cap(NETWATCH_RX, 60);
    drain_all();
    check("unknown ethertype: counted, no event", st().unknown == 1 && netwatch_get_event_count() == 0);

    reset();
    {
        uint32_t l = ipv4(6, ME, GW, 5, 7000, 7001, 0);
        put16(buf + 16, 1400);
        cap(NETWATCH_TX, 14 + 1400);
        (void)l;
    }
    drain_all();
    check("long frame: parsed from the captured prefix", ev(0) && ev(0)->src_port == 7000 && ev(0)->length == 1414);

    reset();
    for (int i = 0; i < 70; i++)
        cap(NETWATCH_TX, ipv4(6, ME, GW, 5, (uint16_t)(2000 + i), 80, 0));
    check("overflow: 6 of 70 dropped and counted", st().dropped == 6 && st().captured == 70);
    check("drain is bounded per call", netobserve_drain() == 16);
    drain_all();
    check("overflow: the 64 queued frames all recorded, oldest first", netwatch_get_event_count() == 64 && ev(0)->src_port == 2000 && ev(63)->src_port == 2063);

    // null and empty captures ignored
    reset();
    netobserve_capture(NETWATCH_RX, 0, 60);
    netobserve_capture(NETWATCH_RX, buf, 0);
    check("null/empty capture ignored", st().captured == 0);

    printf("%d failure(s)\n", fails);
    return fails;
}