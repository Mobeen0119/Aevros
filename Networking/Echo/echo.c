#include "echo.h"
#include "../Compass/ip_directory.h"
#include "../../Lib/kprintf.h"
#include "../../Lib/string.h"
#include "../Rolodex/rolodex.h"
#include "../Bailiff/bailiff.h"

static uint32_t accepted, rejected;

typedef struct
{
    uint8_t src_ip[4];
    uint16_t identifier;
    uint16_t sequence;
    uint16_t payload_len;
    uint8_t payload[ECHO_MAX_PAYLOAD];
} pending_ping_t;

static pending_ping_t queue[ECHO_MAX_PENDING];
static uint16_t head, tail, pending_count;

static uint8_t last_reply_frame[6 + 6 + 2 + 20 + ICMP_MIN_HEADER + ECHO_MAX_PAYLOAD];
static uint16_t last_reply_len;

static uint16_t icmp_checksum(const uint8_t *data, uint16_t len)
{
    uint32_t sum = 0;

    for (int i = 0; i < len; i += 2)
    {
        uint16_t word = (uint16_t)((data[i] << 8) | (i + 1 < len ? data[i + 1] : 0));
        sum += word;
    }

    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    return (uint16_t)(0xFFFF - sum);
}

static icmp_verdict_t echo_check(const uint8_t *payload, uint16_t len)
{

    if (len < ICMP_MIN_HEADER)
        return ICMP_REJECT_TOO_SHORT;

    uint32_t sum = 0;

    for (int i = 0; i < len; i += 2)
    {
        uint16_t word = (uint16_t)((payload[i] << 8) | (i + 1 < len ? payload[i + 1] : 0));
        sum += word;
    }

    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    if (sum != 0xFFFF)
        return ICMP_REJECT_BAD_CHECKSUM;

    uint8_t type = payload[0];
    if (type != ICMP_ECHO_REQUEST)
        return ICMP_REJECT_NOT_ECHO_REQUEST;

    uint16_t payload_len = (uint16_t)(len - ICMP_MIN_HEADER);

    if (payload_len > ECHO_MAX_PAYLOAD)
        return ICMP_REJECT_PAYLOAD_TOO_LARGE;

    return ICMP_ACCEPT;
}

void echo_handle(const uint8_t *payload, uint16_t len, const uint8_t src_ip[4], const uint8_t dst_ip[4])
{
    icmp_verdict_t v = echo_check(payload, len);

    if (v != ICMP_ACCEPT)
    {
        kprintf("[Echo] rejected: %s\n", icmp_verdict_string(v));
        rejected++;
        return;
    }
    accepted++;

    uint16_t identifier = (uint16_t)((payload[4] << 8) | payload[5]);

    uint16_t seq = (uint16_t)((payload[6] << 8) | payload[7]);

    uint16_t payload_len = (uint16_t)(len - ICMP_MIN_HEADER);

    kprintf("[Echo] ping from %d.%d.%d.%d, id=%d seq=%d, %d bytes\n",
            src_ip[0], src_ip[1], src_ip[2], src_ip[3], identifier, seq, payload_len);

    if (pending_count >= ECHO_MAX_PENDING)
    {
        kprintf("[Echo] reply queue full, dropping this one - too many unanswered pings backed up\n");
        return;
    }

    pending_ping_t *p = &queue[tail];
    memcpy(p->src_ip, src_ip, 4);

    p->identifier = identifier;
    p->sequence = seq;

    p->payload_len = payload_len;

    if (payload_len > 0)
        memcpy(p->payload, payload + ICMP_MIN_HEADER, payload_len);

    tail = (uint16_t)((tail + 1) % ECHO_MAX_PENDING);

    pending_count++;
}

int echo_build_reply(uint8_t *out_buf, uint16_t *out_len, uint8_t reply_dst_ip[4])
{
    if (pending_count == 0)
        return 0;

    pending_ping_t *p = &queue[head];

    out_buf[0] = ICMP_ECHO_REPLY;
    out_buf[1] = 0;
    out_buf[2] = 0;
    out_buf[3] = 0;

    out_buf[4] = p->identifier >> 8;
    out_buf[5] = p->identifier & 0xFF;
    out_buf[6] = p->sequence >> 8;
    out_buf[7] = p->sequence & 0xFF;

    if (p->payload_len > 0)
        memcpy(out_buf + ICMP_MIN_HEADER, p->payload, p->payload_len);

    uint16_t total_len = (uint16_t)(ICMP_MIN_HEADER + p->payload_len);
    uint16_t csum = icmp_checksum(out_buf, total_len);

    out_buf[2] = csum >> 8;
    out_buf[3] = csum & 0xFF;

    memcpy(reply_dst_ip, p->src_ip, 4);

    *out_len = total_len;

    head = (uint16_t)((head + 1) % ECHO_MAX_PENDING);
    pending_count--;

    return 1;
}

int echo_dispatch_reply(const uint8_t our_mac[6], const uint8_t our_ip[4], uint32_t *out_pass_id)
{
    uint8_t icmp[ICMP_MIN_HEADER + ECHO_MAX_PAYLOAD];
    uint16_t icmp_len;
    uint8_t dest_ip[4];

    if (!echo_build_reply(icmp, &icmp_len, dest_ip))
        return 0;

    uint8_t dest_mac[6];
    if (!rolodex_lookup(dest_ip, dest_mac))
    {
        kprintf("[Echo] have a reply ready for %d.%d.%d.%d but do not know their MAC yet ... cannot frame it, dropping rather than guessing\n",
                dest_ip[0], dest_ip[1], dest_ip[2], dest_ip[3]);
        return 0;
    }

    uint16_t ip_total = (uint16_t)(20 + icmp_len);
    uint8_t *frame = last_reply_frame;

    memcpy(frame, dest_mac, 6);
    memcpy(frame + 6, our_mac, 6);
    frame[12] = 0x08;
    frame[13] = 0x00;

    uint8_t *ip = frame + 14;
    ip[0] = 0x45;
    ip[1] = 0;
    ip[2] = ip_total >> 8;
    ip[3] = ip_total & 0xFF;
    ip[4] = 0; ip[5] = 0; ip[6] = 0; ip[7] = 0;
    ip[8] = 64;
    ip[9] = 1; 
    ip[10] = 0; ip[11] = 0;
    memcpy(ip + 12, our_ip, 4);
    memcpy(ip + 16, dest_ip, 4);

    uint32_t ip_sum = 0;
    for (int i = 0; i < 20; i += 2)
    {
        uint16_t word = (uint16_t)((ip[i] << 8) | ip[i + 1]);
        ip_sum += word;
    }
    while (ip_sum >> 16) ip_sum = (ip_sum & 0xFFFF) + (ip_sum >> 16);
    uint16_t ip_csum = (uint16_t)(0xFFFF - ip_sum);
    ip[10] = ip_csum >> 8;
    ip[11] = ip_csum & 0xFF;

    memcpy(frame + 14 + 20, icmp, icmp_len);

    uint16_t total_frame_len = (uint16_t)(14 + 20 + icmp_len);
    last_reply_len = total_frame_len;
    return bailiff_request_pass(frame, total_frame_len, out_pass_id);
}

const uint8_t *echo_last_frame(void)
{
    return last_reply_frame;
}

uint16_t echo_last_len(void)
{
    return last_reply_len;
}

uint32_t echo_accepted_count(void)
{
    return accepted;
}

uint32_t echo_rejected_count(void)
{
    return rejected;
}

const char *icmp_verdict_string(icmp_verdict_t v)
{
    switch (v)
    {
    case ICMP_ACCEPT:
        return "ACCEPT";
    case ICMP_REJECT_TOO_SHORT:
        return "REJECT (too short)";
    case ICMP_REJECT_BAD_CHECKSUM:
        return "REJECT (checksum failed)";
    case ICMP_REJECT_NOT_ECHO_REQUEST:
        return "REJECT (not an echo request - we don't originate pings without a transmit path, and don't respond to anything else)";
    case ICMP_REJECT_PAYLOAD_TOO_LARGE:
        return "REJECT (payload too large)";
    default:
        return "REJECT (unknown)";
    }
}

IP_DIRECTORY_ENTRY(1, echo_handle, "Echo (ICMP)");