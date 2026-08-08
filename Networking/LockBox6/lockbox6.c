#include "lockbox6.h"
#include "../../Lib/kprintf.h"
#include "../../Lib/string.h"

static lockbox6_entry_t table[LOCKBOX6_CAPACITY];
static uint32_t rejected_count;
static uint32_t generation[LOCKBOX6_CAPACITY];

static uint32_t count_for_ip(const uint8_t ip[16])
{
    int n = 0;
    for (int i = 0; i < LOCKBOX6_CAPACITY; i++)
        if (table[i].in_use && memcmp(table[i].remote_ip, ip, 16) == 0)
            n++;

    return n;
}

uint32_t lockbox6_next_for_port(uint16_t local_port, uint8_t protocol, uint32_t after_id)
{
    uint32_t start = (after_id >= LOCKBOX6_CAPACITY) ? 0 : after_id + 1;

    for (uint32_t i = start; i < LOCKBOX6_CAPACITY; i++)
    {
        if (table[i].in_use && table[i].local_port == local_port && table[i].protocol == protocol)
            return i;
    }
    return LOCKBOX6_CAPACITY;
}

uint32_t lockbox6_find_connection(uint16_t local_port, const uint8_t remote_ip[16], uint16_t remote_port, uint8_t protocol)
{
    for (uint32_t i = 0; i < LOCKBOX6_CAPACITY; i++)
    {
        if (!table[i].in_use)
            continue;
        if (table[i].local_port == local_port &&
            table[i].remote_port == remote_port &&
            table[i].protocol == protocol &&
            memcmp(table[i].remote_ip, remote_ip, 16) == 0)
            return i;
    }
    return LOCKBOX6_CAPACITY;
}

lockbox6_result_t lockbox6_claim(uint16_t local_port, const uint8_t remote_ip[16], uint16_t remote_port,
                                 uint8_t protocol, uint32_t *out_id)
{
    if (lockbox6_find_connection(local_port, remote_ip, remote_port, protocol) != LOCKBOX6_CAPACITY)
    {
        kprintf("[Lockbox6] rejected: socket already claimed for this exact pair\n");
        rejected_count++;
        return LOCKBOX6_REJECT_ALREADY_EXISTS;
    }

    if (count_for_ip(remote_ip) >= LOCKBOX6_MAX_PER_IP)
    {
        kprintf("[Lockbox6] rejected: remote already holds the max %d sockets\n", LOCKBOX6_MAX_PER_IP);
        rejected_count++;
        return LOCKBOX6_REJECT_IP_QUOTA;
    }

    for (int i = 0; i < LOCKBOX6_CAPACITY; i++)
    {
        if (!table[i].in_use)
        {
            table[i].local_port = local_port;
            memcpy(table[i].remote_ip, remote_ip, 16);
            table[i].remote_port = remote_port;
            table[i].protocol = protocol;
            table[i].buffered_bytes = 0;
            generation[i]++;
            table[i].in_use = 1;
            *out_id = i;

            kprintf("[Lockbox6] claimed slot %d: port %d <-> [ipv6]:%d\n", i, local_port, remote_port);

            return LOCKBOX6_OK;
        }
    }
    kprintf("[Lockbox6] rejected: table full, %d sockets already active\n", LOCKBOX6_CAPACITY);
    rejected_count++;
    return LOCKBOX6_REJECT_TABLE_FULL;
}

int lockbox6_deposit(uint32_t id, uint16_t bytes)
{
    if (id >= LOCKBOX6_CAPACITY || !table[id].in_use)
        return 0;

    if ((table[id].buffered_bytes + bytes) >= LOCKBOX6_MAX_BUFFERED)
    {
        kprintf("[Lockbox6] slot %d refused %d bytes, would exceed the %d byte cap\n",
                id, bytes, LOCKBOX6_MAX_BUFFERED);
        rejected_count++;
        return 0;
    }

    table[id].buffered_bytes += bytes;
    return 1;
}

void lockbox6_consume(uint32_t id, uint16_t bytes)
{
    if (id >= LOCKBOX6_CAPACITY)
        return;

    if (bytes > table[id].buffered_bytes)
        table[id].buffered_bytes = 0;
    else
        table[id].buffered_bytes -= bytes;
}

void lockbox6_release(uint32_t id)
{
    if (id < LOCKBOX6_CAPACITY)
        table[id].in_use = 0;
}

uint32_t lockbox6_active_count(void)
{
    uint32_t n = 0;
    for (int i = 0; i < LOCKBOX6_CAPACITY; i++)
        if (table[i].in_use)
            n++;
    return n;
}

uint32_t lockbox6_rejected_count(void)
{
    return rejected_count;
}

lockbox6_result_t lockbox6_listen(uint16_t local_port, uint8_t protocol, uint32_t *out_id)
{
    uint8_t zero_ip[16] = {0};

    return lockbox6_claim(local_port, zero_ip, 0, protocol, out_id);
}

uint32_t lockbox6_get_generation(uint32_t id)
{
    if (id >= LOCKBOX6_CAPACITY)
        return 0;

    return generation[id];
}

uint32_t lockbox6_find_listener(uint16_t local_port, uint8_t protocol)
{
    for (uint32_t i = 0; i < LOCKBOX6_CAPACITY; i++)
        if (table[i].in_use && table[i].protocol == protocol && table[i].local_port == local_port)
            return i;
    return LOCKBOX6_CAPACITY;
}

int lockbox6_get_tuple(uint32_t id, uint16_t *local_port, uint8_t remote_ip[16], uint16_t *remote_port)
{
    if (id >= LOCKBOX6_CAPACITY || !table[id].in_use)
        return 0;

    *local_port = table[id].local_port;
    memcpy(remote_ip, table[id].remote_ip, 16);
    *remote_port = table[id].remote_port;

    return 1;
}

const char *lockbox6_result_string(lockbox6_result_t r)
{
    switch (r)
    {
    case LOCKBOX6_OK:
        return "OK";
    case LOCKBOX6_REJECT_TABLE_FULL:
        return "REJECT (table full)";
    case LOCKBOX6_REJECT_IP_QUOTA:
        return "REJECT (per-IP quota)";
    case LOCKBOX6_REJECT_ALREADY_EXISTS:
        return "REJECT (already claimed)";
    default:
        return "REJECT (unknown)";
    }
}