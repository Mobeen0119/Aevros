#include "echo6.h"
#include "../Compass6/ip6_directory.h"
#include "../Rolodex6/rolodex6.h"
#include "../Bailiff/bailiff.h"
#include "../FrontDesk/frontdesk.h"
#include "../../Lib/kprintf.h"
#include "../../Lib/string.h"

static uint32_t accepted, rejected;

typedef struct
{
    uint8_t src_ip[16];
    uint16_t identifier;
    uint16_t sequence;
    uint16_t payload_len;
    uint8_t payload[ECHO6_MAX_PAYLOAD];

} pending_ping6_t;

static pending_ping6_t queue[ECHO6_MAX_PENDING];
static uint16_t head, tail, pending_count;

static int have_ra;
static uint8_t ra_prefix[16];
static uint8_t ra_prefix_len;
static uint8_t ra_router_ip[16];

static uint8_t dad_watch_ip[16];
static int dad_watching;
static int dad_conflict;

void echo6_dad_watch(const uint8_t target_ip[16])
{
    memcpy(dad_watch_ip, target_ip, 16);
    dad_watching = 1;
    dad_conflict = 0;
}

int echo6_dad_conflict(void)
{
    return dad_conflict;
}

void echo6_dad_clear(void)
{
    dad_watching = 0;
    dad_conflict = 0;
}

static uint8_t last_reply_frame[6 + 6 + 2 + 40 + ICMP6_MIN_HEADER + ECHO6_MAX_PAYLOAD];
static uint16_t last_reply_len;

static uint16_t icmp6_checksum(const uint8_t *src_ip, const uint8_t *dst_ip, const uint8_t *msg, uint16_t msg_len)
{
    uint32_t sum = 0;

    for (int i = 0; i < 16; i += 2)
        sum += (uint16_t)((src_ip[i] << 8) | src_ip[i + 1]);
    for (int i = 0; i < 16; i += 2)
        sum += (uint16_t)((dst_ip[i] << 8) | dst_ip[i + 1]);

    sum += (msg_len >> 16) & 0xFFFF;
    sum += msg_len & 0xFFFF;
    sum += 58;

    for (int i = 0; i < msg_len; i += 2)
        sum += (uint16_t)((msg[i] << 8) | (i + 1 < msg_len ? msg[i + 1] : 0));

    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    return (uint16_t)(0xFFFF - sum);
}

static void send_neighbor_advertisement(const uint8_t solicitor_ip[16], const uint8_t solicitor_mac[6],const uint8_t our_ip[16], const uint8_t our_mac[6])
{
    static uint8_t frame[6 + 6 + 2 + 40 + ICMP6_MIN_HEADER + 4 + 16 + 8];

    memcpy(frame, solicitor_mac, 6);
    memcpy(frame + 6, our_mac, 6);

    frame[12] = 0x86;
    frame[13] = 0xDD;

    uint8_t *ip = frame + 14;

    uint16_t msg_len = (uint16_t)(ICMP6_MIN_HEADER + 4 + 16 + 8); // hdr+flags+target+tlla option

    ip[0] = 0x60;
    ip[1] = 0;
    ip[2] = 0;

    ip[3] = 0;
    ip[4] = msg_len >> 8;
    ip[5] = msg_len & 0xFF;

    ip[6] = 58; // ICMPv6
    ip[7] = 64;

    memcpy(ip + 8, our_ip, 16);
    memcpy(ip + 24, solicitor_ip, 16);

    uint8_t *icmp = ip + 40;
    icmp[0] = ICMP6_NEIGHBOR_ADVERTISEMENT;
    icmp[1] = 0;
    icmp[2] = 0;
    icmp[3] = 0;

    icmp[4] = 0x60; // Solicited + Override flags
    icmp[5] = 0;
    icmp[6] = 0;
    icmp[7] = 0;

    memcpy(icmp + 8, our_ip, 16);
    icmp[24] = 2;
    icmp[25] = 1;
    memcpy(icmp + 26, our_mac, 6);

    uint16_t csum = icmp6_checksum(our_ip, solicitor_ip, icmp, msg_len);
    icmp[2] = csum >> 8;

    icmp[3] = csum & 0xFF;

    uint16_t total_len = (uint16_t)(14 + 40 + msg_len);

    uint32_t pass_id;

    if (bailiff_request_pass(frame, total_len, &pass_id))
        bailiff_present_pass(pass_id, frame, total_len);
}

static icmp6_verdict_t echo6_check(const uint8_t *payload, uint16_t len, const uint8_t src_ip[16], const uint8_t dst_ip[16])
{
    if (len < ICMP6_MIN_HEADER)
        return ICMP6_REJECT_TOO_SHORT;

    if (icmp6_checksum(src_ip, dst_ip, payload, len) != 0)
        return ICMP6_REJECT_BAD_CHECKSUM;

    uint8_t type = payload[0];

    if (type != ICMP6_ECHO_REQUEST && type != ICMP6_NEIGHBOR_SOLICITATION &&
        type != ICMP6_NEIGHBOR_ADVERTISEMENT && type != ICMP6_ROUTER_ADVERTISEMENT)
        return ICMP6_REJECT_UNHANDLED_TYPE;

    if (type == ICMP6_ECHO_REQUEST && (len - ICMP6_MIN_HEADER) > ECHO6_MAX_PAYLOAD)
        return ICMP6_REJECT_PAYLOAD_TOO_LARGE;

    return ICMP6_ACCEPT;
}

void echo6_handle(const uint8_t *payload, uint16_t len, const uint8_t src_ip[16], const uint8_t dst_ip[16])
{

    icmp6_verdict_t v = echo6_check(payload, len, src_ip, dst_ip);

    if (v != ICMP6_ACCEPT)
    {
        if (v != ICMP6_REJECT_UNHANDLED_TYPE)
            kprintf("[Echo6] rejected: %s\n", icmp6_verdict_string(v));

        rejected++;
        return;
    }
    accepted++;

    uint8_t type = payload[0];

    if (type == ICMP6_ECHO_REQUEST)
    {

        uint16_t identifier = (uint16_t)((payload[4] << 8) | payload[5]);

        uint16_t seq = (uint16_t)((payload[6] << 8) | payload[7]);

        uint16_t payload_len = (uint16_t)(len - ICMP6_MIN_HEADER);

        kprintf("[Echo6] ping6 from source, id=%d seq=%d, %d bytes\n", identifier, seq, payload_len);

        if (pending_count >= ECHO6_MAX_PENDING)
        {
            kprintf("[Echo6] reply queue full, dropping this one\n");
            return;
        }

        pending_ping6_t *p = &queue[tail];
        memcpy(p->src_ip, src_ip, 16);

        p->identifier = identifier;
        p->sequence = seq;

        p->payload_len = payload_len;

        if (payload > 0)
            memcpy(p->payload, payload + ICMP6_MIN_HEADER, payload_len);

        tail = (uint16_t)((tail + 1) % ECHO6_MAX_PENDING);

        pending_count++;
        return;
    }
    if (type == ICMP6_NEIGHBOR_SOLICITATION)
    {
        if (!rolodex6_have_ip())
            return;

        uint8_t our_ip[16];

        rolodex6_get_ip(our_ip);

        const uint8_t *target = payload + 8;
        if (memcmp(target, our_ip, 16) != 0)
            return;

        if (len >= 24 + 8 && payload[24] == 1 && payload[25] == 1)
        {

            const uint8_t *solicitor_mac = payload + 26;

            rolodex6_learn(src_ip, solicitor_mac);

            send_neighbor_advertisement(src_ip, solicitor_mac, our_ip, frontdesk_get_state()->mac);
        }
        return;
    }
    if (type == ICMP6_NEIGHBOR_ADVERTISEMENT)
    {
        if (dad_watching && len >= 24 && memcmp(payload + 8, dad_watch_ip, 16) == 0)
        {
            dad_conflict = 1;
            kprintf("[Echo6] DAD conflict: another node already has the address being checked\n");
        }

        if (len >= 24 + 8 && payload[24] == 2 && payload[25] == 1)
            rolodex6_learn(src_ip, payload + 26);

        return;
    }
    if (type == ICMP6_ROUTER_ADVERTISEMENT)
    {
        if (len < 16)
            return;

        uint16_t router_lifetime = (uint16_t)((payload[6] << 8) | payload[7]);

        uint16_t i = 16;
        while (i + 2 <= len)
        {
            uint8_t opt_type = payload[i];
            uint8_t opt_len_units = payload[i + 1];

            if (opt_len_units == 0)
                break; // a zero-length option would spin forever 

            uint16_t opt_len_bytes = (uint16_t)(opt_len_units * 8);
            if (i + opt_len_bytes > len)
                break;

            if (opt_type == 3 && opt_len_bytes == 32)
            {
                uint8_t prefix_len_bits = payload[i + 2];
                uint8_t flags = payload[i + 3];
                int on_link = (flags & 0x80) != 0;
                int autonomous = (flags & 0x40) != 0;

                if (on_link && autonomous && prefix_len_bits <= 128)
                {
                    memcpy(ra_prefix, payload + i + 16, 16);
                    ra_prefix_len = prefix_len_bits;
                    memcpy(ra_router_ip, src_ip, 16);
                    have_ra = (router_lifetime > 0);

                    kprintf("[Echo6] got a Router Advertisement with an on-link/autonomous /%d prefix\n", prefix_len_bits);
                }
            }

            i = (uint16_t)(i + opt_len_bytes);
        }
        return;
    }
}

int echo6_have_router_advertisement(void)
{
    return have_ra;
}

void echo6_get_prefix(uint8_t out_prefix[16], uint8_t *out_prefix_len)
{
    memcpy(out_prefix, ra_prefix, 16);
    *out_prefix_len = ra_prefix_len;
}

void echo6_get_router(uint8_t out_router_ip[16])
{
    memcpy(out_router_ip, ra_router_ip, 16);
}

int echo6_dispatch_router_solicitation(const uint8_t our_mac[6], const uint8_t our_ip[16], uint32_t *out_pass_id)
{
    static const uint8_t all_routers[16] = {0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2};
    static const uint8_t unspecified[16] = {0};

    int include_slla = memcmp(our_ip, unspecified, 16) != 0;
    uint16_t msg_len = (uint16_t)(ICMP6_MIN_HEADER + (include_slla ? 8 : 0));

    static uint8_t frame[6 + 6 + 2 + 40 + ICMP6_MIN_HEADER + 8];

    frame[0] = 0x33;
    frame[1] = 0x33;
    frame[2] = 0x00;
    frame[3] = 0x00;
    frame[4] = 0x00;
    frame[5] = 0x02;
    memcpy(frame + 6, our_mac, 6);
    frame[12] = 0x86;
    frame[13] = 0xDD;

    uint8_t *ip = frame + 14;
    ip[0] = 0x60;
    ip[1] = 0;
    ip[2] = 0;
    ip[3] = 0;
    ip[4] = msg_len >> 8;
    ip[5] = msg_len & 0xFF;
    ip[6] = 58;
    ip[7] = 255; 
    memcpy(ip + 8, our_ip, 16);
    memcpy(ip + 24, all_routers, 16);

    uint8_t *icmp = ip + 40;
    icmp[0] = ICMP6_ROUTER_SOLICITATION;
    icmp[1] = 0;
    icmp[2] = 0;
    icmp[3] = 0;
    icmp[4] = 0;
    icmp[5] = 0;
    icmp[6] = 0;
    icmp[7] = 0;

    if (include_slla)
    {
        icmp[8] = 1; // Source Link-Layer Address option 
        icmp[9] = 1; // length in 8-byte units 
        memcpy(icmp + 10, our_mac, 6);
    }

    uint16_t csum = icmp6_checksum(our_ip, all_routers, icmp, msg_len);
    icmp[2] = csum >> 8;
    icmp[3] = csum & 0xFF;

    uint16_t total_len = (uint16_t)(14 + 40 + msg_len);

    if (bailiff_request_pass(frame, total_len, out_pass_id))
        return bailiff_present_pass(*out_pass_id, frame, total_len);

    return 0;
}

int echo6_dispatch_neighbor_solicitation(const uint8_t target_ip[16], const uint8_t our_mac[6],const uint8_t our_ip[16], uint32_t *out_pass_id)
{
    static const uint8_t unspecified[16] = {0};

    int include_slla = memcmp(our_ip, unspecified, 16) != 0;
    uint16_t msg_len = (uint16_t)(24 + (include_slla ? 8 : 0));   // 24 = type/code/cksum/reserved/target 

    static uint8_t frame[6 + 6 + 2 + 40 + 24 + 8];

    uint8_t dest_ip[16] = {0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0xff,
                            target_ip[13], target_ip[14], target_ip[15]};

    frame[0] = 0x33;
    frame[1] = 0x33;
    frame[2] = 0xff;
    frame[3] = target_ip[13];
    frame[4] = target_ip[14];
    frame[5] = target_ip[15];
    memcpy(frame + 6, our_mac, 6);
    frame[12] = 0x86;
    frame[13] = 0xDD;

    uint8_t *ip = frame + 14;
    ip[0] = 0x60;
    ip[1] = 0;
    ip[2] = 0;
    ip[3] = 0;
    ip[4] = msg_len >> 8;
    ip[5] = msg_len & 0xFF;
    ip[6] = 58;
    ip[7] = 255;
    memcpy(ip + 8, our_ip, 16);
    memcpy(ip + 24, dest_ip, 16);

    uint8_t *icmp = ip + 40;
    icmp[0] = ICMP6_NEIGHBOR_SOLICITATION;
    icmp[1] = 0;
    icmp[2] = 0;
    icmp[3] = 0;
    icmp[4] = 0; // reserved 
    icmp[5] = 0;
    icmp[6] = 0;
    icmp[7] = 0;
    memcpy(icmp + 8, target_ip, 16);

    if (include_slla)
    {
        icmp[24] = 1; // Source Link-Layer Address option 
        icmp[25] = 1;
        memcpy(icmp + 26, our_mac, 6);
    }

    uint16_t csum = icmp6_checksum(our_ip, dest_ip, icmp, msg_len);
    icmp[2] = csum >> 8;
    icmp[3] = csum & 0xFF;

    uint16_t total_len = (uint16_t)(14 + 40 + msg_len);

    if (bailiff_request_pass(frame, total_len, out_pass_id))
        return bailiff_present_pass(*out_pass_id, frame, total_len);

    return 0;
}

int echo6_dispatch_reply(const uint8_t our_mac[6], const uint8_t our_ip[16], uint32_t *out_pass_id)
{

    if (pending_count == 0)
        return 0;

    pending_ping6_t *p = &queue[head];
    uint8_t icmp[ICMP6_MIN_HEADER + ECHO6_MAX_PAYLOAD];

    icmp[0] = ICMP6_ECHO_REPLY;

    icmp[1] = 0;
    icmp[2] = 0;

    icmp[3] = 0;

    icmp[4] = p->identifier >> 8;
    icmp[5] = p->identifier & 0xFF;
    icmp[6] = p->sequence >> 8;
    icmp[7] = p->sequence & 0xFF;

    if (p->payload_len > 0)
        memcpy(icmp + ICMP6_MIN_HEADER, p->payload, p->payload_len);

    uint16_t msg_len = (uint16_t)(ICMP6_MIN_HEADER + p->payload_len);

    uint8_t dest_mac[6];
    if (!rolodex6_lookup(p->src_ip, dest_mac))
    {
        kprintf("[Echo6] have a reply ready but don't know the neighbor's MAC yet, dropping rather than guessing\n");
        head = (uint16_t)((head + 1) % ECHO6_MAX_PENDING);
        pending_count--;
        return 0;
    }

    uint8_t *frame = last_reply_frame;
    memcpy(frame, dest_mac, 6);
    memcpy(frame + 6, our_mac, 6);
    frame[12] = 0x86;
    frame[13] = 0xDD;

    uint8_t *ip = frame + 14;
    ip[0] = 0x60;
    ip[1] = 0;
    ip[2] = 0;

    ip[3] = 0;
    ip[4] = msg_len >> 8;
    ip[5] = msg_len & 0xFF;

    ip[6] = 58;
    ip[7] = 64;
    memcpy(ip + 8, our_ip, 16);
    memcpy(ip + 24, p->src_ip, 16);

    uint16_t csum = icmp6_checksum(our_ip, p->src_ip, icmp, msg_len);
    icmp[2] = csum >> 8;
    icmp[3] = csum & 0xFF;

    memcpy(ip + 40, icmp, msg_len);

    last_reply_len = (uint16_t)(14 + 40 + msg_len);

    head = (uint16_t)((head + 1) % ECHO6_MAX_PENDING);
    pending_count--;

    return bailiff_request_pass(frame, last_reply_len, out_pass_id);
}

const uint8_t *echo6_last_frame(void)
{
    return last_reply_frame;
}

uint16_t echo6_last_len(void)
{
    return last_reply_len;
}

uint32_t echo6_accepted_count(void)
{
    return accepted;
}

uint32_t echo6_rejected_count(void)
{
    return rejected;
}

const char *icmp6_verdict_string(icmp6_verdict_t v)
{
    switch (v)
    {
    case ICMP6_ACCEPT:
        return "ACCEPT";
    case ICMP6_REJECT_TOO_SHORT:
        return "REJECT (too short)";
    case ICMP6_REJECT_BAD_CHECKSUM:
        return "REJECT (checksum failed)";
    case ICMP6_REJECT_UNHANDLED_TYPE:
        return "not handled (not echo request/NS/NA)";
    case ICMP6_REJECT_PAYLOAD_TOO_LARGE:
        return "REJECT (payload too large)";
    default:
        return "REJECT (unknown)";
    }
}

IP6_DIRECTORY_ENTRY(58, echo6_handle, "Echo6 (ICMPv6)");