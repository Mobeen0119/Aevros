#include "conversation6.h"
#include "../Compass6/ip6_directory.h"
#include "../LockBox6/lockbox6.h"
#include "rapport6.h"
#include "../Inbox6/inbox6.h"
#include "../Rolodex6/rolodex6.h"
#include "../Bailiff/bailiff.h"
#include "../Menu/menu.h"
#include "../Scheduler6/scheduler6.h"
#include "../WayStation6/waystation6.h"
#include "../SynCookie6/syncookie6.h"
#include "../../Lib/string.h"
#include "../../Lib/kprintf.h"
#include "../FrontDesk/frontdesk.h"
#include "../../kernel/Process/task.h"


#define TCP6_MAX_PAYLOAD 1440 // Ethernet MTU(1500) - IPv6(40) - TCP header(20) 
#define TCP6_MIN_HEADER 20

#define FLAG_FIN 0x1
#define FLAG_SYN 0x2

#define FLAG_RST 0x4
#define FLAG_PSH 0x8
#define FLAG_ACK 0x10
#define FLAG_URG 0x20

static uint32_t accepted, rejected;

static uint8_t last_dispatched_frame[14 + 40 + 20 + TCP6_MAX_PAYLOAD];
static uint16_t last_dispatched_len;

static int checksum_ok(const uint8_t *tcp, uint16_t tcp_len, const uint8_t src_ip[16], const uint8_t dst_ip[16])
{
    uint32_t sum = 0;

    for (int i = 0; i < 16; i += 2)
        sum += (uint16_t)((src_ip[i] << 8) | src_ip[i + 1]);
    for (int i = 0; i < 16; i += 2)
        sum += (uint16_t)((dst_ip[i] << 8) | dst_ip[i + 1]);

    sum += (tcp_len >> 16) & 0xFFFF;
    sum += tcp_len & 0xFFFF;
    sum += 6;

    for (int i = 0; i < tcp_len; i += 2)
    {
        uint32_t word = (uint16_t)((tcp[i] << 8) | (i + 1 < tcp_len ? tcp[i + 1] : 0));
        sum += word;
    }
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    return sum == 0xFFFF;
}

static uint16_t compute_tcp6_checksum(const uint8_t *tcp, uint16_t tcp_len, const uint8_t src_ip[16], const uint8_t dst_ip[16])
{
    uint32_t sum = 0;

    for (int i = 0; i < 16; i += 2)
        sum += (uint16_t)((src_ip[i] << 8) | src_ip[i + 1]);
    for (int i = 0; i < 16; i += 2)
        sum += (uint16_t)((dst_ip[i] << 8) | dst_ip[i + 1]);

    sum += (tcp_len >> 16) & 0xFFFF;
    sum += tcp_len & 0xFFFF;
    sum += 6;

    for (int i = 0; i < tcp_len; i += 2)
    {
        uint32_t word = (uint16_t)((tcp[i] << 8) | (i + 1 < tcp_len ? tcp[i + 1] : 0));
        sum += word;
    }
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    return (uint16_t)(0xFFFF - sum);
}

static int flag_make_sense(uint8_t flags)
{
    if ((flags & FLAG_SYN) && (flags & FLAG_FIN))
        return 0;
    if (flags == 0)
        return 0;
    return 1;
}

static uint32_t draw_isn6(const uint8_t local_ip[16], uint16_t local_port, const uint8_t remote_ip[16], uint16_t remote_port)
{
    uint32_t mix = get_ticks();

    mix = mix * 2654435761u;
    mix ^= ((uint32_t)local_ip[12] << 24) | ((uint32_t)local_ip[13] << 16) | ((uint32_t)local_ip[14] << 8) | local_ip[15];
    mix ^= ((uint32_t)remote_ip[12] << 24) | ((uint32_t)remote_ip[13] << 16) | ((uint32_t)remote_ip[14] << 8) | remote_ip[15];
    mix ^= ((uint32_t)local_port << 16) | remote_port;
    mix = mix * 2246822519u + 3266489917u;

    return mix;
}


static void send_cookie_syn_ack6(const uint8_t peer_ip[16], uint16_t peer_port,const uint8_t our_ip[16], uint16_t our_port,
                                  uint32_t cookie_isn, uint32_t peer_isn, const uint8_t our_mac[6])
{
    uint8_t dest_mac[6];
    if (!rolodex6_lookup(peer_ip, dest_mac))
        return; /* Compass6 already learned this peer's MAC from the SYN itself, but bail cleanly if it somehow hasn't */

    static uint8_t frame[6 + 6 + 2 + 40 + TCP6_MIN_HEADER];

    memcpy(frame, dest_mac, 6);
    memcpy(frame + 6, our_mac, 6);
    frame[12] = 0x86;
    frame[13] = 0xDD;

    uint8_t *ip = frame + 14;
    ip[0] = 0x60;
    ip[1] = 0;
    ip[2] = 0;
    ip[3] = 0;
    ip[4] = TCP6_MIN_HEADER >> 8;
    ip[5] = TCP6_MIN_HEADER & 0xFF;
    ip[6] = 6;
    ip[7] = 64;
    memcpy(ip + 8, our_ip, 16);
    memcpy(ip + 24, peer_ip, 16);

    uint8_t *tcp = ip + 40;
    tcp[0] = our_port >> 8;
    tcp[1] = our_port & 0xFF;
    tcp[2] = peer_port >> 8;
    tcp[3] = peer_port & 0xFF;

    tcp[4] = (uint8_t)(cookie_isn >> 24);
    tcp[5] = (uint8_t)(cookie_isn >> 16);
    tcp[6] = (uint8_t)(cookie_isn >> 8);
    tcp[7] = (uint8_t)cookie_isn;

    uint32_t ack = peer_isn + 1;
    tcp[8] = (uint8_t)(ack >> 24);
    tcp[9] = (uint8_t)(ack >> 16);
    tcp[10] = (uint8_t)(ack >> 8);
    tcp[11] = (uint8_t)ack;

    tcp[12] = 5 << 4;
    tcp[13] = FLAG_SYN | FLAG_ACK;


    tcp[14] = (LOCKBOX6_MAX_BUFFERED >> 8) & 0xFF;
    tcp[15] = LOCKBOX6_MAX_BUFFERED & 0xFF;
    tcp[16] = 0;
    tcp[17] = 0;
    tcp[18] = 0;
    tcp[19] = 0;

    uint16_t tcp_csum = compute_tcp6_checksum(tcp, TCP6_MIN_HEADER, our_ip, peer_ip);
    tcp[16] = tcp_csum >> 8;
    tcp[17] = tcp_csum & 0xFF;

    uint16_t total_len = (uint16_t)(14 + 40 + TCP6_MIN_HEADER);
    uint32_t pass_id;
    if (bailiff_request_pass(frame, total_len, &pass_id))
        bailiff_present_pass(pass_id, frame, total_len);
}

static tcp_verdict_t conversation6_check(const uint8_t *payload, uint16_t length,const uint8_t src_ip[16], const uint8_t dst_ip[16])
{
    if (length < TCP6_MIN_HEADER)
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

void conversation6_handle(const uint8_t *payload, uint16_t length, const uint8_t src_ip[16], const uint8_t dst_ip[16])
{
    tcp_verdict_t v = conversation6_check(payload, length, src_ip, dst_ip);

    if (v != TCP_ACCEPT)
    {
        kprintf("[Conversation6] rejected: %s\n", tcp_verdict_string(v));
        rejected++;
        return;
    }
    accepted++;

    uint16_t src_port = (uint16_t)((payload[0] << 8) | payload[1]);
    uint16_t dst_port = (uint16_t)((payload[2] << 8) | payload[3]);
    uint8_t flags = payload[13];

    kprintf("[Conversation6] accepted, port %d -> port %d, flags=%x%s%s%s\n",
            src_port, dst_port, flags,
            (flags & FLAG_SYN) ? " SYN (Start)" : "",
            (flags & FLAG_ACK) ? " ACK (Recieved)" : "",
            (flags & FLAG_FIN) ? " FIN (End)" : "");

    if (!menu_is_open(dst_port, 6))
    {
        kprintf("[Conversation6] port %d isn't on the Menu, refusing regardless of any listener\n", dst_port);
        return;
    }

    uint32_t listener = lockbox6_find_listener(dst_port, 6);
    if (listener == LOCKBOX6_CAPACITY)
    {
        kprintf("[Conversation6] nobody's listening on port %d, discarding\n", dst_port);
        return;
    }

    uint32_t seq = (uint32_t)((payload[4] << 24) | (payload[5] << 16) | (payload[6] << 8) | payload[7]);

    uint32_t existing_conn = lockbox6_find_connection(dst_port, src_ip, src_port, 6);

    if (existing_conn != LOCKBOX6_CAPACITY && rapport6_get_state(existing_conn) == CONV_SYN_SENT)
    {
        if (flags & FLAG_RST)
        {
            kprintf("[Conversation6] slot %d: connection refused\n", existing_conn);
            rapport6_on_rst(existing_conn);
            return;
        }
        if ((flags & FLAG_SYN) && (flags & FLAG_ACK))
        {
            uint32_t ack_num = (uint32_t)((payload[8] << 24) | (payload[9] << 16) | (payload[10] << 8) | payload[11]);
            rapport6_on_syn_ack(existing_conn, seq, ack_num);
            return;
        }
        kprintf("[Conversation6] slot %d: unexpected segment while waiting on syn-ack, ignoring\n", existing_conn);
        return;
    }

    if (flags & FLAG_SYN)
    {
        uint8_t our_mac[6];
        memcpy(our_mac, frontdesk_get_state()->mac, 6);

        if (syncookie6_should_activate())
        {
            uint32_t cookie_isn = syncookie6_generate(src_ip, src_port, dst_ip, dst_port);

            send_cookie_syn_ack6(src_ip, src_port, dst_ip, dst_port, cookie_isn, seq, our_mac);

            kprintf("[Conversation6] under load, sent cookie syn-ack without a slot\n");
            return;
        }

        uint32_t conn_id;
        lockbox6_result_t r = lockbox6_claim(dst_port, src_ip, src_port, 6, &conn_id);
        if (r != LOCKBOX6_OK)
        {
            kprintf("[Conversation6] SYN refused: %s\n", lockbox6_result_string(r));
            return;
        }

        uint32_t our_isn = draw_isn6(dst_ip, dst_port, src_ip, src_port);
        rapport6_on_syn(conn_id, seq, our_isn);
        uint32_t pass_id;
        if (conversation6_dispatch_syn_ack(conn_id, our_mac, dst_ip, &pass_id))
            bailiff_present_pass(pass_id, conversation6_last_frame(), conversation6_last_len());

        kprintf("[Conversation6] SYN accepted into its own slot %d, syn-ack sent\n", conn_id);
        return;
    }

    uint8_t data_off = (uint8_t)((payload[12] >> 4) & 0x0F);
    uint16_t header_len = (uint16_t)(data_off * 4);

    uint32_t conn_id = lockbox6_find_connection(dst_port, src_ip, src_port, 6);

    if (conn_id == LOCKBOX6_CAPACITY)
    {
        if ((flags & FLAG_ACK) && !(flags & FLAG_SYN))
        {
            uint32_t ack_num = (uint32_t)((payload[8] << 24) | (payload[9] << 16) | (payload[10] << 8) | payload[11]);

            if (syncookie6_validate(src_ip, src_port, dst_ip, dst_port, ack_num))
            {
                lockbox6_result_t r = lockbox6_claim(dst_port, src_ip, src_port, 6, &conn_id);
                if (r == LOCKBOX6_OK)
                {
                    uint32_t peer_isn = seq - 1;
                    uint32_t cookie_isn = ack_num - 1;
                    rapport6_on_syn(conn_id, peer_isn, cookie_isn);
                    rapport6_on_ack(conn_id, ack_num);
                    kprintf("[Conversation6] slot %d: cookie handshake completed, slot allocated now that it's proven real\n", conn_id);
                }
                else
                {
                    kprintf("[Conversation6] cookie validated but couldn't claim a slot: %s\n", lockbox6_result_string(r));
                    return;
                }
            }
            else
            {
                kprintf("[Conversation6] non-SYN segment with no known connection, discarding\n");
                return;
            }
        }
        else
        {
            kprintf("[Conversation6] non-SYN segment with no known connection, discarding\n");
            return;
        }
    }

    uint16_t peer_window = (uint16_t)((payload[14] << 8) | payload[15]);
    rapport6_set_peer_window(conn_id, peer_window);

    if (!rapport6_seq_expected(conn_id, seq))
    {
        uint16_t probe_len = (uint16_t)(length - header_len);

        if (rapport6_seq_is_stale_retransmit(conn_id, seq, probe_len))
            kprintf("[Conversation6] slot %d: seq=%u is an old retransmission, already have this - ignoring, not an attack\n", conn_id, seq);
        else if (probe_len > 0)
            waystation6_hold(conn_id, seq, payload + header_len, probe_len);
        else
            kprintf("[Conversation6] slot %d: seq=%u doesn't match expected, refusing to trust it\n", conn_id, seq);
        return;
    }

    if (flags & FLAG_RST)
    {
        rapport6_on_rst(conn_id);
        return;
    }

    if (flags & FLAG_FIN)
    {
        rapport6_on_fin(conn_id);
        return;
    }

    if (flags & FLAG_ACK)
    {
        uint32_t ack_num = (uint32_t)((payload[8] << 24) | (payload[9] << 16) | (payload[10] << 8) | payload[11]);

        rapport6_on_ack(conn_id, ack_num);
        scheduler6_ack(conn_id, ack_num);
    }

    uint16_t data_len = (uint16_t)(length - header_len);
    if (data_len > 0 && inbox6_deposit(conn_id, payload + header_len, data_len))
    {
        rapport6_advance_seq(conn_id, data_len);
        waystation6_drain(conn_id);
    }
}

uint32_t tcp6_accepted_count(void)
{
    return accepted;
}

uint32_t tcp6_rejected_count(void)
{
    return rejected;
}

IP6_DIRECTORY_ENTRY(6, conversation6_handle, "Conversation6 (TCP6) ");

int conversation6_dispatch(uint32_t conn_id, uint8_t flags, uint32_t seq, uint32_t ack, const uint8_t *payload, uint16_t payload_len, const uint8_t our_mac[6], const uint8_t our_ip[16], uint32_t *out_pass_id)
{
    if (payload_len > TCP6_MAX_PAYLOAD)
        return 0;

    if (!flag_make_sense(flags))
        return 0;

    uint16_t local_port;
    uint8_t remote_ip[16];
    uint16_t remote_port;

    if (!lockbox6_get_tuple(conn_id, &local_port, remote_ip, &remote_port))
        return 0;

    uint8_t dest_mac[6];
    if (!rolodex6_lookup(remote_ip, dest_mac))
    {
        kprintf("[Conversation6] slot %d: no MAC cached for the peer yet, dropping rather than guessing (no Foyer6)\n", conn_id);
        return 0;
    }

    uint8_t *frame = last_dispatched_frame;
    memcpy(frame, dest_mac, 6);
    memcpy(frame + 6, our_mac, 6);
    frame[12] = 0x86;
    frame[13] = 0xDD;

    uint8_t *ip = frame + 14;
    uint16_t tcp_len = (uint16_t)(TCP6_MIN_HEADER + payload_len);

    ip[0] = 0x60;
    ip[1] = 0;
    ip[2] = 0;
    ip[3] = 0;

    ip[4] = tcp_len >> 8;
    ip[5] = tcp_len & 0xFF;
    ip[6] = 6; // next header = TCP 
    ip[7] = 64; // hop limit 
    memcpy(ip + 8, our_ip, 16);
    memcpy(ip + 24, remote_ip, 16);

    uint8_t *tcp = ip + 40;
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
    uint16_t recv_window = waystation6_receive_window(conn_id);
    tcp[14] = (recv_window >> 8) & 0xFF;

    tcp[15] = recv_window & 0xFF;
    tcp[16] = 0;
    tcp[17] = 0;
    tcp[18] = 0;
    tcp[19] = 0;

    if (payload_len)
        memcpy(tcp + 20, payload, payload_len);

    uint16_t tcp_csum = compute_tcp6_checksum(tcp, tcp_len, our_ip, remote_ip);
    tcp[16] = tcp_csum >> 8;
    tcp[17] = tcp_csum & 0xFF;

    last_dispatched_len = (uint16_t)(14 + 40 + tcp_len);

    return bailiff_request_pass(frame, last_dispatched_len, out_pass_id);
}

int conversation6_dispatch_syn_ack(uint32_t conn_id, const uint8_t our_mac[6], const uint8_t our_ip[16], uint32_t *out_pass_id)
{
    uint32_t our_isn = rapport6_get_our_isn(conn_id);
    uint32_t ack = rapport6_get_expected_seq(conn_id);

    if (!conversation6_dispatch(conn_id, FLAG_SYN | FLAG_ACK, our_isn, ack, 0, 0, our_mac, our_ip, out_pass_id))
        return 0;

    scheduler6_track(conn_id, our_isn, last_dispatched_frame, last_dispatched_len);
    return 1;
}

int conversation6_dispatch_data(uint32_t conn_id, const uint8_t *data, uint16_t len,const uint8_t our_mac[6], const uint8_t our_ip[16], uint32_t *out_pass_id)
{
    if (len == 0 || len > TCP6_MAX_PAYLOAD)
        return 0;

    if (rapport6_get_state(conn_id) != CONV_ESTABLISHED)
        return 0;

    if (scheduler6_bytes_in_flight(conn_id) > 0)
    {
        kprintf("[Conversation6] slot %d: already have an unacked segment out, refusing to send another yet\n", conn_id);
        return 0;
    }

    if (!rapport6_send_allowed(conn_id, len))
    {
        kprintf("[Conversation6] slot %d: peer's window won't fit %u more bytes right now\n", conn_id, len);
        return 0;
    }

    uint32_t seq = rapport6_get_send_seq(conn_id);
    uint32_t ack = rapport6_get_expected_seq(conn_id);

    if (!conversation6_dispatch(conn_id, FLAG_ACK, seq, ack, data, len, our_mac, our_ip, out_pass_id))
        return 0;

    scheduler6_track(conn_id, seq, last_dispatched_frame, last_dispatched_len);

    rapport6_advance_send_seq(conn_id, len);

    return 1;
}

int conversation6_dispatch_syn(uint32_t conn_id, uint32_t our_isn, const uint8_t our_mac[6], const uint8_t our_ip[16], uint32_t *out_pass_id)
{
    if (!conversation6_dispatch(conn_id, FLAG_SYN, our_isn, 0, 0, 0, our_mac, our_ip, out_pass_id))
        return 0;

    scheduler6_track(conn_id, our_isn, last_dispatched_frame, last_dispatched_len);
    return 1;
}

const uint8_t *conversation6_last_frame(void)
{
    return last_dispatched_frame;
}

uint16_t conversation6_last_len(void)
{
    return last_dispatched_len;
}

int conversation6_dispatch_fin(uint32_t conn_id, const uint8_t our_mac[6], const uint8_t our_ip[16], uint32_t *out_pass_id)
{
    conversation_state_t state = rapport6_get_state(conn_id);

    if (state != CONV_ESTABLISHED && state != CONV_CLOSE_WAIT)
        return 0;

    if (scheduler6_bytes_in_flight(conn_id) > 0)
    {
        kprintf("[Conversation6] slot %d: still have an unacked segment out, refusing to close yet\n", conn_id);
        return 0;
    }

    uint32_t seq = rapport6_get_send_seq(conn_id);
    uint32_t ack = rapport6_get_expected_seq(conn_id);

    if (!conversation6_dispatch(conn_id, FLAG_FIN | FLAG_ACK, seq, ack, 0, 0, our_mac, our_ip, out_pass_id))
        return 0;

    scheduler6_track(conn_id, seq, last_dispatched_frame, last_dispatched_len);
    rapport6_advance_send_seq(conn_id, 1);
    rapport6_initiate_close(conn_id, seq);

    return 1;
}

int conversation6_dispatch_ack(uint32_t conn_id, const uint8_t our_mac[6], const uint8_t our_ip[16], uint32_t *out_pass_id)
{
    uint32_t our_isn = rapport6_get_our_isn(conn_id);
    uint32_t ack = rapport6_get_expected_seq(conn_id);

    return conversation6_dispatch(conn_id, FLAG_ACK, our_isn, ack, 0, 0, our_mac, our_ip, out_pass_id);
}