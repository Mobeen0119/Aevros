#include "conversation.h"
#include "../Compass/ip_directory.h"
#include "../LockBox/lockbox.h"
#include "rapport.h"
#include "../Inbox/inbox.h"
#include "../Lottery/lottery.h"
#include "../Roldex/rolodex.h"
#include "../Bailiff/bailiff.h"
#include "../Menu/menu.h"
#include "../Scheduler/scheduler.h"
#include "../Waystation/waystation.h"
#include "../../Lib/string.h"
#include "../../Lib/kprintf.h"

#define TCP_MAX_PAYLOAD 1460 // Ethernet MTU(1500)...Ipv4(20)...TCP header(20)
#define TCP_MIN_HEADER 20

#define FLAG_FIN 0x1
#define FLAG_SYN 0x2
#define FLAG_RST 0x4
#define FLAG_PSH 0x8
#define FLAG_ACK 0x10
#define FLAG_URG 0x20

static uint32_t accepted, rejected;

static uint8_t last_dispatched_frame[14 + 20 + 20 + 1460];
static uint16_t last_dispatched_len;

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
    if ((flags & FLAG_SYN) && (flags & FLAG_FIN))
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

    if (!menu_is_open(dst_port, 6))
    {
        kprintf("[Conversation] port %d isn't on the Menu, refusing regardless of any listener\n", dst_port);
        return;
    }

    uint32_t listener = lockbox_find_listener(dst_port, 6);
    if (listener == LOCKBOX_CAPACITY)
    {
        kprintf("[Conversation] nobody's listening on port %d, discarding\n", dst_port);
        return;
    }

    uint32_t seq = (uint32_t)((payload[4] << 24) | (payload[5] << 16) | (payload[6] << 8) | payload[7]);

    uint32_t existing_conn = lockbox_find_connection(dst_port, src_ip, src_port, 6);

    if (existing_conn != LOCKBOX_CAPACITY && rapport_get_state(existing_conn) == CONV_SYN_SENT)
    {
        if (flags & FLAG_RST)
        {
            kprintf("[Conversation] slot %d: connection refused\n", existing_conn);
            rapport_on_rst(existing_conn);
            return;
        }
        if ((flags & FLAG_SYN) && (flags & FLAG_ACK))
        {
            uint32_t ack_num = (uint32_t)((payload[8] << 24) | (payload[9] << 16) | (payload[10] << 8) | payload[11]);
            rapport_on_syn_ack(existing_conn, seq, ack_num);
            return;
        }
        kprintf("[Conversation] slot %d: unexpected segment while waiting on syn-ack, ignoring\n", existing_conn);
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
        uint32_t our_isn = lottery_draw_isn(dst_ip, dst_port, src_ip, src_port);
        rapport_on_syn(conn_id, seq, our_isn);
        kprintf("[Conversation] SYN accepted into its own slot %d\n", conn_id);
        return;
    }

    uint8_t data_off = (uint8_t)((payload[12] >> 4) & 0x0F);
    uint16_t header_len = (uint16_t)(data_off * 4);

    uint32_t conn_id = lockbox_find_connection(dst_port, src_ip, src_port, 6);
    if (conn_id == LOCKBOX_CAPACITY)
    {
        kprintf("[Conversation] non-SYN segment with no known connection, discarding\n");
        return;
    }

    uint16_t peer_window = (uint16_t)((payload[14] << 8) | payload[15]);
    rapport_set_peer_window(conn_id, peer_window);

    if (!rapport_seq_expected(conn_id, seq))
    {
        uint16_t probe_len = (uint16_t)(length - header_len);
        if (rapport_seq_is_stale_retransmit(conn_id, seq, probe_len))
            kprintf("[Conversation] slot %d: seq=%u is an old retransmission, already have this - ignoring, not an attack\n", conn_id, seq);
        else if (probe_len > 0)
            waystation_hold(conn_id, seq, payload + header_len, probe_len);
        else
            kprintf("[Conversation] slot %d: seq=%u doesn't match expected, refusing to trust it\n", conn_id, seq);
        return;
    }

    if (flags & FLAG_RST)
    {
        rapport_on_rst(conn_id);
        return;
    }

    if (flags & FLAG_FIN)
    {
        rapport_on_fin(conn_id);
        return;
    }

    if (flags & FLAG_ACK)
    {
        uint32_t ack_num = (uint32_t)((payload[8] << 24) | (payload[9] << 16) | (payload[10] << 8) | payload[11]);

        rapport_on_ack(conn_id);
        scheduler_ack(conn_id, ack_num);
    }

    uint16_t data_len = (uint16_t)(length - header_len);
    if (data_len > 0 && inbox_deposit(conn_id, payload + header_len, data_len))
    {
        rapport_advance_seq(conn_id, data_len);
        waystation_drain(conn_id);
    }
}

uint32_t tcp_accepted_count(void)
{
    return accepted;
}

uint32_t tcp_rejected_count(void)
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

int conversation_dispatch(uint32_t conn_id, uint8_t flags, uint32_t seq, uint32_t ack, const uint8_t *payload, uint16_t payload_len,
                          const uint8_t our_mac[6], const uint8_t our_ip[4], uint32_t *out_pass_id)
{
    if (payload_len > TCP_MAX_PAYLOAD)
        return 0;

    if (!flag_make_sense(flags))
        return 0;

    uint16_t local_port;
    uint8_t remote_ip[4];
    uint16_t remote_port;

    if (!lockbox_get_tuple(conn_id, &local_port, remote_ip, &remote_port))
        return 0;

    uint8_t dest_mac[6];
    if (!rolodex_lookup(remote_ip, dest_mac))
    {
        kprintf("[Conversation] ready to transmit.....\n",
                conn_id, remote_ip[0], remote_ip[1], remote_ip[2], remote_ip[3]);
        return 0;
    }

    uint8_t *frame = last_dispatched_frame;
    memcpy(frame, dest_mac, 6);
    memcpy(frame + 6, our_mac, 6);
    frame[12] = 0x08;
    frame[13] = 0x00;

    uint8_t *ip = frame + 14;
    uint16_t ip_total = 20 + 20 + payload_len;

    ip[0] = 0x45;
    ip[1] = 0;
    ip[2] = ip_total >> 8;
    ip[3] = ip_total & 0xFF;
    ip[4] = 0;

    ip[5] = 0;
    ip[6] = 0;
    ip[7] = 0;
    ip[8] = 64;
    ip[9] = 6;
    ip[10] = 0;
    ip[11] = 0;
    memcpy(ip + 12, our_ip, 4);
    memcpy(ip + 16, remote_ip, 4);

    uint32_t ip_sum = 0;
    for (int i = 0; i < 20; i += 2)
    {
        uint16_t word = (uint16_t)((ip[i] << 8) | ip[i + 1]);
        ip_sum += word;
    }
    while (ip_sum >> 16)
        ip_sum = (ip_sum & 0xFFFF) + (ip_sum >> 16);

    uint16_t ip_csum = (uint16_t)(0xFFFF - ip_sum);
    ip[10] = ip_csum >> 8;
    ip[11] = ip_csum & 0xFF;

    uint8_t *tcp = ip + 20;
    tcp[0] = local_port >> 8;
    tcp[1] = local_port & 0xFF;

    tcp[2] = remote_port >> 8;
    tcp[3] = remote_port & 0xFF;

    tcp[4] = (uint8_t)(seq >> 24);
    tcp[5] = (uint8_t)(seq >> 16);

    tcp[6] = (uint8_t)(seq >> 8);
    tcp[7] = (uint8_t)seq;

    tcp[8] = (uint8_t)(ack >> 24);
    tcp[9] = (uint8_t)(ack >> 16);

    tcp[10] = (uint8_t)(ack >> 8);
    tcp[11] = (uint8_t)ack;
    tcp[12] = 5 << 4;
    tcp[13] = flags;
    uint16_t recv_window = waystation_receive_window(conn_id);
    tcp[14] = (recv_window >> 8) & 0xFF;
    tcp[15] = recv_window & 0xFF;
    tcp[16] = 0;
    tcp[17] = 0;
    tcp[18] = 0;
    tcp[19] = 0;

    if (payload_len)
        memcpy(tcp + 20, payload, payload_len);

    uint32_t tcp_sum = 0;

    for (int i = 0; i < 4; i += 2)
        tcp_sum += (uint16_t)((our_ip[i] << 8) | our_ip[i + 1]);
    for (int i = 0; i < 4; i += 2)
        tcp_sum += (uint16_t)((remote_ip[i] << 8) | remote_ip[i + 1]);

    tcp_sum += 6;
    tcp_sum += 20 + payload_len;

    const uint16_t tcp_len = TCP_MIN_HEADER + payload_len;

    for (int i = 0; i < tcp_len; i += 2)
    {
        uint16_t word = (uint16_t)((tcp[i] << 8) | (i + 1 < tcp_len ? tcp[i + 1] : 0));
        tcp_sum += word;
    }

    while (tcp_sum >> 16)
        tcp_sum = (tcp_sum & 0xFFFF) + (tcp_sum >> 16);
    uint16_t tcp_csum = (uint16_t)(0xFFFF - tcp_sum);

    tcp[16] = tcp_csum >> 8;
    tcp[17] = tcp_csum & 0xFF;

    last_dispatched_len = (uint16_t)(14 + ip_total);

    return bailiff_request_pass(frame, 14 + ip_total, out_pass_id);
}

int conversation_dispatch_syn_ack(uint32_t conn_id, const uint8_t our_mac[6], const uint8_t our_ip[4], uint32_t *out_pass_id)
{
    uint32_t our_isn = rapport_get_our_isn(conn_id);
    uint32_t ack = rapport_get_expected_seq(conn_id);

    if (!conversation_dispatch(conn_id, FLAG_SYN | FLAG_ACK, our_isn, ack, 0, 0, our_mac, our_ip, out_pass_id))
        return 0;

    scheduler_track(conn_id, our_isn, last_dispatched_frame, last_dispatched_len);
    return 1;
}

int conversation_dispatch_ack(uint32_t conn_id, const uint8_t our_mac[6], const uint8_t our_ip[4], uint32_t *out_pass_id)
{
    uint32_t our_isn = rapport_get_our_isn(conn_id);
    uint32_t ack = rapport_get_expected_seq(conn_id);

    return conversation_dispatch(conn_id, FLAG_ACK, our_isn, ack, 0, 0, our_mac, our_ip, out_pass_id);
}