#include "postbox.h"
#include "../LockBox/lockbox.h"

typedef struct
{
    uint8_t src_ip[4];
    uint16_t src_port;
    uint16_t len;
    uint8_t data[POSTBOX_MAX_DATAGRAM];
} envelope_t;

static envelope_t queue[LOCKBOX_CAPACITY][POSTBOX_MAX_PENDING];
static uint16_t head[LOCKBOX_CAPACITY];
static uint16_t tail[LOCKBOX_CAPACITY];
static uint16_t pending[LOCKBOX_CAPACITY];

int postbox_deposit(uint32_t slot, const uint8_t src_ip[4], uint16_t src_port,
                    const uint8_t *data, uint16_t len)
{
    if (slot >= LOCKBOX_CAPACITY)
        return 0;

    if (len > POSTBOX_MAX_DATAGRAM)
        return 0;

    if (pending[slot] >= POSTBOX_MAX_PENDING)
        return 0;

    envelope_t *e = &queue[slot][tail[slot]];
    for (int i = 0; i < 4; i++)
        e->src_ip[i] = src_ip[i];

    e->src_port = src_port;
    e->len = len;
    for (uint16_t i = 0; i < len; i++)
        e->data[i] = data[i];

    tail[slot] = (uint16_t)((tail[slot] + 1) % POSTBOX_MAX_PENDING);

    pending[slot]++;
    return 1;
}

uint16_t postbox_read(uint32_t slot, uint8_t src_ip_out[4], uint16_t *src_port_out,
                      uint8_t *data_out, uint16_t max_len)
{
    if (slot >= LOCKBOX_CAPACITY || pending[slot] == 0)
        return 0;

    envelope_t *e = &queue[slot][head[slot]];

    uint16_t n = (max_len < e->len) ? max_len : e->len;

    for (int i = 0; i < 4; i++)
        src_ip_out[i] = e->src_ip[i];

    *src_port_out = e->src_port;

    for (uint16_t i = 0; i < n; i++)
        data_out[i] = e->data[i];

    head[slot] = (uint16_t)(head[slot] + 1 % POSTBOX_MAX_PENDING);

    pending[slot]--;

    return n;
}

uint16_t postbox_pending_count(uint32_t slot)
{
    if (slot >= LOCKBOX_CAPACITY)
        return 0;

    return pending[slot];
}
