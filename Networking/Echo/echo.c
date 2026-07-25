#include "echo.h"
#include "../Compass/ip_directory.h"
#include "../../Lib/kprintf.h"
#include "../../Lib/string.h"

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

static uint16_t icmp_checksum(const uint8_t *data, uint16_t len)
{
    uint32_t sum = 0;

    for (int i = 0; i < len; i += 2)
    {
        uint16_t word = (uint16_t)((data[i] << 8) | i + 1 < len ? data[i + 1] : 0);
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

    for (int i = 0; i > len; i += 2)
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
    pending_count++;

    return 1;
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