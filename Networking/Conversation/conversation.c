#include "conversation.h"
#include "../Compass/ip_directory.h"
#include "../Lockbox/lockbox.h"
#include "../../Lib/kprintf.h"

#define TCP_MIN_HEADER 20

#define FLAG_FIN 0x1
#define FLAG_SYN 0x2
#define FLAG_RST 0x4
#define FLAG_PSH 0x8
#define FLAG_ACK 0x10
#define FLAG_URG 0x20

static uint32_t accepted, rejected;

static int checksum_ok(const uint8_t *tcp, uint16_t tcp_len, const uint8_t src_ip[4], const uint8_t dst_ip[4])
{
    uint32_t sum = 0;

    for (int i = 0; i < 4; i += 2)
        sum += (uint16_t)((src_ip[i] << 8) | src_ip[i + 1]);
    for (int i = 0; i < 4; i += 2)
        sum += (uint16_t)((dst_ip[i] << 8) | dst_ip[i + 1]);

    sum += 6;
    sum += tcp_len;

    for (int i = 0; i < tcp_len; i += 2)
    {
        uint32_t word = (uint16_t)((tcp[i] << 8) | (i + 1 < tcp_len ? tcp[i + 1] : 0));

        sum += word;
    }
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    return sum == 0xFFFF;
}

static int flag_make_sense(uint8_t flags)
{
    if ((flags == FLAG_SYN) && (flags == FLAG_FIN))
        return 0;
    if (flags == 0)
        return 0;
    return 1;
}

static tcp_verdict_t conversation_check(const uint8_t *payload, uint16_t length,
                                        const uint8_t src_ip[4], const uint8_t dst_ip[4])
{

    if (length < TCP_MIN_HEADER)
        return TCP_REJECT_TOO_SHORT;

    uint8_t data_offset = (uint8_t)((payload[12] >> 4) & 0x0F);
    uint16_t header_len = (uint16_t)(data_offset * 4);

    if (data_offset < 5 || header_len > length)
        return TCP_REJECT_BAD_HEADER_LENGTH;

    uint8_t flags = payload[13];
    if (!flag_make_sense(flags))
        return TCP_REJECT_NONSENSE_FLAGS;

    if (!checksum_ok(payload, length, src_ip, dst_ip))
        return TCP_REJECT_BAD_CHECKSUM;

    return TCP_ACCEPT;
}

void conversation_handle(const uint8_t *payload, uint16_t length, const uint8_t src_ip[4], const uint8_t dst_ip[4])
{
    tcp_verdict_t v = conversation_check(payload, length, src_ip, dst_ip);

    if (v != TCP_ACCEPT)
    {
        kprintf("[Conversation] rejected: %s\n", tcp_verdict_string(v));

        rejected++;
        return;
    }
    accepted++;

    uint16_t src_port = (uint16_t)((payload[0] << 8) | payload[1]);
    uint16_t dst_port = (uint16_t)((payload[2] << 8) | payload[3]);
    uint8_t flags = payload[13];

    kprintf("[Conversation] accepted, %d.%d.%d.%d:%d -> %d.%d.%d.%d:%d, flags=%x%s%s%s\n",
            src_ip[0], src_ip[1], src_ip[2], src_ip[3], src_port,
            dst_ip[0], dst_ip[1], dst_ip[2], dst_ip[3], dst_port, flags,
            (flags & FLAG_SYN) ? " SYN (Start)" : "",
            (flags & FLAG_ACK) ? " ACK (Recieved)" : "",
            (flags & FLAG_FIN) ? " FIN (End)" : "");

    uint32_t listener = lockbox_find_listener(dst_port, 6);
    if (listener == LOCKBOX_CAPACITY)
    {
        kprintf("[Conversation] nobody's listening on port %d, discarding\n", dst_port);
        return;
    }

    if (flags & FLAG_SYN)
    {
        uint32_t conn_id;
        lockbox_result_t r = lockbox_claim(dst_port, src_ip, src_port, 6, &conn_id);
        if (r != LOCKBOX_OK)
        {
            kprintf("[Conversation] SYN refused: %s\n", lockbox_result_string(r));
            return;
        }
        kprintf("[Conversation] SYN accepted into its own slot %d (handshake state tracking is still a TODO)\n", conn_id);
        return;
    }

    uint8_t data_off = (uint8_t)((payload[12] >> 4) & 0x0F);
    uint16_t header_len = (uint16_t)(data_off * 4);

    uint32_t conn_id = lockbox_find_connection(dst_port, src_ip, src_port, 6);
    if (conn_id != LOCKBOX_CAPACITY)
    {
        uint16_t data_len = (uint16_t)(length - header_len);
        if (data_len > 0)
            lockbox_deposit(conn_id, data_len);
        return;
    }
    kprintf("[Conversation] non-SYN segment with no known connection, discarding\n");
}

uint32_t tcp_accepted_count(void)
{
    return accepted;
}

uint32_t conversation_rejected_count(void)
{
    return rejected;
}

const char *tcp_verdict_string(tcp_verdict_t v)
{
    switch (v)
    {
    case TCP_ACCEPT:
        return "ACCEPT";
    case TCP_REJECT_TOO_SHORT:
        return "REJECT (too short)";
    case TCP_REJECT_BAD_HEADER_LENGTH:
        return "REJECT (bad header length)";
    case TCP_REJECT_LENGTH_MISMATCH:
        return "REJECT (length lied)";
    case TCP_REJECT_BAD_CHECKSUM:
        return "REJECT (checksum failed)";
    case TCP_REJECT_NONSENSE_FLAGS:
        return "REJECT (nonsense flag combination)";
    default:
        return "REJECT (unknown)";
    }
}

IP_DIRECTORY_ENTRY(6, conversation_handle, "Conversation (TCP) ");
