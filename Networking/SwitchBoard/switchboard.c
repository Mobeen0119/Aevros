#include "switchboard.h"
#include "../LockBox/lockbox.h"
#include "../Conversation/rapport.h"
#include "../../Lib/kprintf.h"
#include "../Inbox/inbox.h"

static uint32_t accepted_generation[LOCKBOX_CAPACITY];

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

uint16_t switchboard_recv_udp(uint16_t port, uint8_t *out, uint16_t max_len)
{
    uint32_t slot = lockbox_find_listener(port, 17);

    if (slot == LOCKBOX_CAPACITY)
        return 0;

    return inbox_read(slot, out, max_len);
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