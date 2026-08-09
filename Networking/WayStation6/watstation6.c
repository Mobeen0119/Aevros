#include "waystation6.h"
#include "../LockBox6/lockbox6.h"
#include "../Inbox6/inbox6.h"
#include "../Conversation6/rapport6.h"
#include "../../Lib/string.h"
#include "../../Lib/kprintf.h"

typedef struct
{
    uint32_t seq;
    uint16_t len;
    int in_use;
    uint8_t data[WAYSTATION6_MAX_SEGMENT];
} held_segment6_t;

static held_segment6_t slots[LOCKBOX6_CAPACITY][WAYSTATION6_MAX_PENDING];
static uint32_t seen_generation[LOCKBOX6_CAPACITY];

static int valid_id(uint32_t conn_id)
{
    return conn_id < LOCKBOX6_CAPACITY;
}

static void reset_if_stale(uint32_t conn_id)
{
    uint32_t gen = lockbox6_get_generation(conn_id);

    if (seen_generation[conn_id] != gen)
    {
        for (int i = 0; i < WAYSTATION6_MAX_PENDING; i++)
            slots[conn_id][i].in_use = 0;

        seen_generation[conn_id] = gen;
    }
}

int waystation6_hold(uint32_t conn_id, uint32_t seq, const uint8_t *payload, uint16_t len)
{
    if (!valid_id(conn_id) || len == 0 || len > WAYSTATION6_MAX_SEGMENT)
        return 0;

    reset_if_stale(conn_id);

    held_segment6_t *row = slots[conn_id];
    int free_slots = -1;

    for (int i = 0; i < WAYSTATION6_MAX_PENDING; i++)
    {
        if (row[i].in_use && row[i].seq == seq)
        {
            kprintf("[Waystation6] slot %u: seq %u already held, ignoring the echo\n", conn_id, seq);
            return 1;
        }

        if (!row[i].in_use && free_slots < 0)
            free_slots = i;
    }
    if (free_slots < 0)
    {
        kprintf("[Waystation6] slot %u: holding area full, dropping out-of-order seq %u\n", conn_id, seq);
        return 0;
    }

    row[free_slots].seq = seq;
    row[free_slots].len = len;

    memcpy(row[free_slots].data, payload, len);
    row[free_slots].in_use = 1;

    kprintf("[Waystation6] slot %u: holding out-of-order seq %u (%u bytes) until the gap closes\n", conn_id, seq, len);

    return 1;
}

int waystation6_drain(uint32_t conn_id)
{
    if (!valid_id(conn_id))
        return 0;

    reset_if_stale(conn_id);

    held_segment6_t *row = slots[conn_id];
    int drained = 0;

    for (;;)
    {
        uint32_t expected = rapport6_get_expected_seq(conn_id);
        int found = -1;
        for (int i = 0; i < WAYSTATION6_MAX_PENDING; i++)
        {
            if (row[i].in_use && row[i].seq == expected)
            {
                found = i;
                break;
            }
        }
        if (found < 0)
            break;

        if (!inbox6_deposit(conn_id, row[found].data, row[found].len))
        {
            kprintf("[Waystation6] slot %u: Inbox6 has no room, seq %u stays held for now\n", conn_id, row[found].seq);
            break;
        }

        rapport6_advance_seq(conn_id, row[found].len);
        kprintf("[Waystation6] slot %u: gap closed, delivered seq %u (%u bytes)\n", conn_id, row[found].seq, row[found].len);

        row[found].in_use = 0;
        drained++;
    }
    return drained;
}

uint16_t waystation6_receive_window(uint32_t conn_id)
{
    if (!valid_id(conn_id))
        return 0;

    reset_if_stale(conn_id);

    uint32_t free_space = LOCKBOX6_MAX_BUFFERED - inbox6_available(conn_id);

    uint32_t held_bytes = 0;

    for (int i = 0; i < WAYSTATION6_MAX_PENDING; i++)
    {
        if (slots[conn_id][i].in_use)
            held_bytes += slots[conn_id][i].len;
    }

    if (held_bytes >= free_space)
        return 0;

    uint32_t window = free_space - held_bytes;

    return (window > 0xFFFF) ? 0xFFFF : (uint16_t)window;
}

uint16_t waystation6_pending_count(uint32_t conn_id)
{
    if (!valid_id(conn_id))
        return 0;

    reset_if_stale(conn_id);

    uint16_t n = 0;

    for (int i = 0; i < WAYSTATION6_MAX_PENDING; i++)
        if (slots[conn_id][i].in_use)
            n++;

    return n;
}