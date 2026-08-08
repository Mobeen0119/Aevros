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

static void send_neighbor_advertisement(const uint8_t solicitor_ip[16], const uint8_t solicitor_mac[6],
                                        const uint8_t our_ip[16], const uint8_t our_mac[6])
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

    if (type != ICMP6_ECHO_REQUEST && type != ICMP6_NEIGHBOR_SOLICITATION && type != ICMP6_NEIGHBOR_ADVERTISEMENT)
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
        if (len >= 24 + 8 && payload[24] == 2 && payload[25] == 1)
            rolodex6_learn(src_ip, payload + 26);

        return;
    }
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