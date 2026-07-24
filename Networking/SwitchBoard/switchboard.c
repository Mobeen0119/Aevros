#include "switchboard.h"
#include "../LockBox/lockbox.h"
#include "../Conversation/rapport.h"
#include "../../Lib/kprintf.h"

static uint32_t accepted_generation[LOCKBOX_CAPACITY];

int switchboard_bind(uint16_t port)
{
    uint32_t id;
    lockbox_result_t r = lockbox_listen(port, 6, &id);

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

    while ((id = lockbox_next_for_port(port, 6, &id)) != LOCKBOX_CAPACITY)
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