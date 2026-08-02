#include "switchboard.h"
#include "../LockBox/lockbox.h"
#include "../Conversation/rapport.h"
#include "../Inbox/inbox.h"
#include "../Postbox/postbox.h"
#include "../../Lib/kprintf.h"
#include "../Lottery/lottery.h"
#include "../Bailiff/bailiff.h"

#define EPHEMERAL_PORT_LOW 49152
#define EPHEMERAL_PORT_HIGH 65535
static uint16_t next_ephemeral = EPHEMERAL_PORT_LOW;
static uint32_t accepted_generation[LOCKBOX_CAPACITY];

static int pick_ephemeral_port(void)
{

    for (uint32_t tries = 0; tries < (EPHEMERAL_PORT_HIGH - EPHEMERAL_PORT_LOW); tries++)
    {
        uint16_t candidate = next_ephemeral;
        next_ephemeral = (next_ephemeral >= EPHEMERAL_PORT_HIGH) ? EPHEMERAL_PORT_LOW : (uint16_t)(next_ephemeral + 1);

        if (lockbox_find_listener(candidate, 6) == LOCKBOX_CAPACITY)
            return candidate;
    }
    return 0;
}

int switchboard_connect(const uint8_t dest_ip[4], uint16_t dest_port,
                        const uint8_t our_mac[6], const uint8_t our_ip[4], uint32_t *out_conn_id)
{
    uint16_t local_port = pick_ephemeral_port();
    if (local_port == 0)
    {
        kprintf("[Switchboard] connect refused: no ephemeral ports free\n");
        return 0;
    }

    uint32_t conn_id;
    lockbox_result_t r = lockbox_claim(local_port, dest_ip, dest_port, 6, &conn_id);

    if (r != LOCKBOX_OK)
    {
        kprintf("[Switchboard] connect to %d.%d.%d.%d:%d refused: %s\n",
                dest_ip[0], dest_ip[1], dest_ip[2], dest_ip[3], dest_port, lockbox_result_string(r));
        return 0;
    }

    uint32_t our_isn = lottery_draw_isn(our_ip, local_port, dest_ip, dest_port);

    rapport_initiate_connect(conn_id, our_isn);

    uint32_t pass_id;
    if (conversation_dispatch_syn(conn_id, our_isn, our_mac, our_ip, &pass_id))
        bailiff_present_pass(pass_id, conversation_last_frame(), conversation_last_len());

    kprintf("[Switchboard] slot %d: reaching out to %d.%d.%d.%d:%d from local port %d\n",
            conn_id, dest_ip[0], dest_ip[1], dest_ip[2], dest_ip[3], dest_port, local_port);
    *out_conn_id = conn_id;
    return 1;
}

int switchboard_send(uint32_t conn_id, const uint8_t *data, const uint16_t len, const uint8_t our_mac[6], const uint8_t our_ip[4])
{

    uint32_t pass_id;

    if (!conversation_dispatch_data(conn_id, data, len, our_mac, our_ip, &pass_id))
        return 0;

    if (!bailiff_present_pass(pass_id, conversation_last_frame(), conversation_last_len()))
        return 0;

    kprintf("[Switchboard] slot %d: sent %u bytes\n", conn_id, len);

    return 1;
}

int switchboard_close(uint32_t conn_id, const uint8_t our_mac[6], const uint8_t our_ip[4])
{
    uint32_t pass_id;

    if (!conversation_dispatch_fin(conn_id, our_mac, our_ip, &pass_id))
        return 0;

    if (!bailiff_present_pass(pass_id, conversation_last_frame(), conversation_last_len()))
        return 0;

    kprintf("[Switchboard] slot %d: closing\n", conn_id);

    return 1;
}

int switchboard_bind(uint16_t port, uint8_t protocol)
{
    uint32_t id;
    lockbox_result_t r = lockbox_listen(port, protocol, &id);

    if (r != LOCKBOX_OK)
    {
        kprintf("[Switchboard] bind on port %d refused: %s\n", port, lockbox_result_string(r));
        return 0;
    }

    kprintf("[Switchboard] now taking calls on port %d\n", port);
    return 1;
}

uint32_t switchboard_accept(uint16_t port)
{
    uint32_t id = LOCKBOX_CAPACITY;

    while ((id = lockbox_next_for_port(port, 6, id)) != LOCKBOX_CAPACITY)
    {
        uint32_t gen = lockbox_get_generation(id);

        if (accepted_generation[id] == gen)
            continue;

        if (rapport_get_state(id) == CONV_ESTABLISHED)
        {
            accepted_generation[id] = gen;
            kprintf("[Switchboard] slot %d picked up on port %d\n", id, port);

            return id;
        }
    }
    return SWITCHBOARD_NONE;
}

uint16_t switchboard_recv(uint32_t conn_id, uint8_t *out, uint16_t max_len)
{
    if (conn_id >= LOCKBOX_CAPACITY)
        return 0;

    if (accepted_generation[conn_id] != lockbox_get_generation(conn_id))
    {
        kprintf("[Switchboard] recv on slot %d refused: not a connection we handed out\n", conn_id);
        return 0;
    }

    return inbox_read(conn_id, out, max_len);
}

uint16_t switchboard_recv_udp(uint16_t port, uint8_t src_ip_out[4], uint16_t *src_port_out,
                              uint8_t *data_out, uint16_t max_len)
{
    uint32_t slot = lockbox_find_listener(port, 17);

    if (slot == LOCKBOX_CAPACITY)
        return 0;

    return postbox_read(slot, src_ip_out, src_port_out, data_out, max_len);
}