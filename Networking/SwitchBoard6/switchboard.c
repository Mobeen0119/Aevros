
#include "switchboard6.h"
#include "../LockBox6/lockbox6.h"
#include "../Conversation6/rapport6.h"
#include "../Conversation6/conversation6.h"
#include "../Inbox6/inbox6.h"
#include "../PostCard6/postcard6.h"
#include "../Bailiff/bailiff.h"
#include "../../kernel/Process/task.h"
#include "../../Lib/kprintf.h"

#define EPHEMERAL6_PORT_LOW 49152
#define EPHEMERAL6_PORT_HIGH 65535
static uint16_t next_ephemeral6 = EPHEMERAL6_PORT_LOW;
static uint32_t accepted_generation[LOCKBOX6_CAPACITY];

static int pick_ephemeral6_port(void)
{
    for (uint32_t tries = 0; tries < (EPHEMERAL6_PORT_HIGH - EPHEMERAL6_PORT_LOW); tries++)
    {
        uint16_t candidate = next_ephemeral6;
        next_ephemeral6 = (next_ephemeral6 >= EPHEMERAL6_PORT_HIGH) ? EPHEMERAL6_PORT_LOW : (uint16_t)(next_ephemeral6 + 1);

        if (lockbox6_find_listener(candidate, 6) == LOCKBOX6_CAPACITY)
            return candidate;
    }
    return 0;
}

static uint32_t draw_isn6_outbound(const uint8_t local_ip[16], uint16_t local_port, const uint8_t remote_ip[16], uint16_t remote_port)
{
    uint32_t mix = get_ticks() ^ 0x9E3779B9u;

    mix = mix * 2654435761u;
    mix ^= ((uint32_t)local_ip[12] << 24) | ((uint32_t)local_ip[13] << 16) | ((uint32_t)local_ip[14] << 8) | local_ip[15];
    mix ^= ((uint32_t)remote_ip[12] << 24) | ((uint32_t)remote_ip[13] << 16) | ((uint32_t)remote_ip[14] << 8) | remote_ip[15];
    mix ^= ((uint32_t)local_port << 16) | remote_port;
    mix = mix * 2246822519u + 3266489917u;

    return mix;
}

int switchboard6_connect(const uint8_t dest_ip[16], uint16_t dest_port,const uint8_t our_mac[6], const uint8_t our_ip[16], uint32_t *out_conn_id)
{
    uint16_t local_port = pick_ephemeral6_port();
    if (local_port == 0)
    {
        kprintf("[Switchboard6] connect refused: no ephemeral ports free\n");
        return 0;
    }

    uint32_t conn_id;
    lockbox6_result_t r = lockbox6_claim(local_port, dest_ip, dest_port, 6, &conn_id);

    if (r != LOCKBOX6_OK)
    {
        kprintf("[Switchboard6] connect on port %d refused: %s\n", dest_port, lockbox6_result_string(r));
        return 0;
    }

    uint32_t our_isn = draw_isn6_outbound(our_ip, local_port, dest_ip, dest_port);

    rapport6_initiate_connect(conn_id, our_isn);

    uint32_t pass_id;
    if (conversation6_dispatch_syn(conn_id, our_isn, our_mac, our_ip, &pass_id))
        bailiff_present_pass(pass_id, conversation6_last_frame(), conversation6_last_len());

    kprintf("[Switchboard6] slot %d: reaching out to port %d from local port %d\n", conn_id, dest_port, local_port);
    *out_conn_id = conn_id;
    return 1;
}

int switchboard6_send(uint32_t conn_id, const uint8_t *data, uint16_t len, const uint8_t our_mac[6], const uint8_t our_ip[16])
{
    uint32_t pass_id;

    if (!conversation6_dispatch_data(conn_id, data, len, our_mac, our_ip, &pass_id))
        return 0;

    if (!bailiff_present_pass(pass_id, conversation6_last_frame(), conversation6_last_len()))
        return 0;

    kprintf("[Switchboard6] slot %d: sent %u bytes\n", conn_id, len);

    return 1;
}

int switchboard6_close(uint32_t conn_id, const uint8_t our_mac[6], const uint8_t our_ip[16])
{
    uint32_t pass_id;

    if (!conversation6_dispatch_fin(conn_id, our_mac, our_ip, &pass_id))
        return 0;

    if (!bailiff_present_pass(pass_id, conversation6_last_frame(), conversation6_last_len()))
        return 0;

    kprintf("[Switchboard6] slot %d: closing\n", conn_id);

    return 1;
}

int switchboard6_bind(uint16_t port)
{
    uint32_t id;
    lockbox6_result_t r = lockbox6_listen(port, 6, &id);

    if (r != LOCKBOX6_OK)
    {
        kprintf("[Switchboard6] bind on port %d refused: %s\n", port, lockbox6_result_string(r));
        return 0;
    }

    kprintf("[Switchboard6] now taking calls on port %d\n", port);
    return 1;
}

uint32_t switchboard6_accept(uint16_t port)
{
    uint32_t id = LOCKBOX6_CAPACITY;

    while ((id = lockbox6_next_for_port(port, 6, id)) != LOCKBOX6_CAPACITY)
    {
        uint32_t gen = lockbox6_get_generation(id);

        if (accepted_generation[id] == gen)
            continue;

        if (rapport6_get_state(id) == CONV_ESTABLISHED)
        {
            accepted_generation[id] = gen;
            kprintf("[Switchboard6] slot %d picked up on port %d\n", id, port);

            return id;
        }
    }
    return SWITCHBOARD6_NONE;
}

uint16_t switchboard6_recv(uint32_t conn_id, uint8_t *out, uint16_t max_len)
{
    if (conn_id >= LOCKBOX6_CAPACITY)
        return 0;

    if (accepted_generation[conn_id] != lockbox6_get_generation(conn_id))
    {
        kprintf("[Switchboard6] recv on slot %d refused: not a connection we handed out\n", conn_id);
        return 0;
    }

    return inbox6_read(conn_id, out, max_len);
}

int switchboard6_send_udp(uint16_t local_port, const uint8_t dest_ip[16], uint16_t dest_port,const uint8_t our_mac[6], const uint8_t our_ip[16],const uint8_t *data, uint16_t len)
{
    return postcard6_dispatch(dest_ip, dest_port, local_port, our_mac, our_ip, data, len);
}