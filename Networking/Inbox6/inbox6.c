#include "inbox6.h"
#include "../LockBox6/lockbox6.h"

static uint8_t storage[LOCKBOX6_CAPACITY][LOCKBOX6_MAX_BUFFERED];
static uint16_t write_pos[LOCKBOX6_CAPACITY];
static uint16_t read_pos[LOCKBOX6_CAPACITY];
static uint16_t count[LOCKBOX6_CAPACITY];
static uint32_t seen_generation[LOCKBOX6_CAPACITY];

static void reset_if_stale(uint32_t conn_id)
{
    uint32_t gen = lockbox6_get_generation(conn_id);

    if (seen_generation[conn_id] != gen)
    {
        write_pos[conn_id] = 0;
        read_pos[conn_id] = 0;
        count[conn_id] = 0;
        seen_generation[conn_id] = gen;
    }
}

int inbox6_deposit(uint32_t conn_id, const uint8_t *payload, uint16_t length)
{
    if (conn_id >= LOCKBOX6_CAPACITY)
        return 0;

    reset_if_stale(conn_id);

    if (!lockbox6_deposit(conn_id, length))
        return 0;

    for (uint16_t i = 0; i < length; i++)
    {
        storage[conn_id][write_pos[conn_id]] = payload[i];
        write_pos[conn_id] = (uint16_t)((write_pos[conn_id] + 1) % LOCKBOX6_MAX_BUFFERED);
    }

    count[conn_id] = (uint16_t)(count[conn_id] + length);
    return 1;
}

uint16_t inbox6_read(uint32_t conn_id, uint8_t *out, uint16_t max_len)
{
    if (conn_id >= LOCKBOX6_CAPACITY)
        return 0;

    reset_if_stale(conn_id);

    uint16_t n = (max_len < count[conn_id]) ? max_len : count[conn_id];

    for (uint16_t i = 0; i < n; i++)
    {
        out[i] = storage[conn_id][read_pos[conn_id]];
        read_pos[conn_id] = (uint16_t)((read_pos[conn_id] + 1) % LOCKBOX6_MAX_BUFFERED);
    }

    count[conn_id] = (uint16_t)(count[conn_id] - n);

    if (n > 0)
        lockbox6_consume(conn_id, n);

    return n;
}

uint16_t inbox6_available(uint32_t conn_id)
{
    if (conn_id >= LOCKBOX6_CAPACITY)
        return 0;

    reset_if_stale(conn_id);

    return count[conn_id];
}
