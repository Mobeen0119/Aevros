#include "lockbox.h"
#include "../../Lib/kprintf.h"
#include "../../Lib/string.h"

static lockbox_entry_t table[LOCKBOX_CAPACITY];
static uint32_t rejected_count;

static uint32_t count_for_ip(const uint8_t ip[4])
{
    int n = 0;
    for (int i = 0; i < LOCKBOX_CAPACITY; i++)
        if (table[i].in_use && memcmp(table[i].remote_ip, ip, 4) == 0)
            n++;

    return n;
}

uint32_t lockbox_find_connection(uint16_t local_port, const uint8_t remote_ip[4], uint16_t remote_port, uint8_t protocol)
{
    for (uint32_t i = 0; i < LOCKBOX_CAPACITY; i++)
    {
        if (!table[i].in_use)
            continue;
        if (table[i].local_port == local_port &&
            table[i].remote_port == remote_port &&
            table[i].protocol == protocol &&
            memcmp(table[i].remote_ip, remote_ip, 4) == 0)
            return i;
    }
    return LOCKBOX_CAPACITY;
}

lockbox_result_t lockbox_claim(uint16_t local_port, const uint8_t remote_ip[4], uint16_t remote_port,
                               uint8_t protocol, uint32_t *out_id)
{
    if (lockbox_find_connection(local_port, remote_ip, remote_port, protocol != LOCKBOX_CAPACITY))
    {

        kprintf("[Lockbox] rejected: socket already claimed for this exact pair\n");
        rejected_count++;
        return LOCKBOX_REJECT_ALREADY_EXISTS;
    }

    if (count_for_ip(remote_ip) >= LOCKBOX_MAX_PER_IP)
    {
        kprintf("[Lockbox] rejected: %d.%d.%d.%d already holds the max %d sockets\n",
                remote_ip[0], remote_ip[1], remote_ip[2], remote_ip[3], LOCKBOX_MAX_PER_IP);
        rejected_count++;
        return LOCKBOX_REJECT_IP_QUOTA;
    }

    for (int i = 0; i < LOCKBOX_CAPACITY; i++)
    {
        if (!table[i].in_use)
        {
            table[i].local_port = local_port;
            memcpy(table[i].remote_ip, remote_ip, 4);
            table[i].remote_port = remote_port;
            table[i].protocol = protocol;
            table[i].buffered_bytes = 0;
            table[i].in_use = 1;
            *out_id = i;

            kprintf("[Lockbox] claimed slot %d: port %d <-> %d.%d.%d.%d:%d\n",
                    i, local_port, remote_ip[0], remote_ip[1], remote_ip[2], remote_ip[3], remote_port);

            return 0;
        }
    }
    kprintf("[Lockbox] rejected: table full, %d sockets already active\n", LOCKBOX_CAPACITY);
    rejected_count++;
    return LOCKBOX_REJECT_TABLE_FULL;
}

int lockbox_deposit(uint32_t id, uint16_t bytes)
{
    if (id >= LOCKBOX_CAPACITY || !table[id].in_use)
        return 0;

    if ((table[id].buffered_bytes + bytes) >= LOCKBOX_MAX_BUFFERED)
    {
        kprintf("[Lockbox] slot %d refused %d bytes, would exceed the %d byte cap, peer isn't being read fast enough or is flooding\n",
                id, bytes, LOCKBOX_MAX_BUFFERED);
        rejected_count++;
        return 0;
    }

    table[id].buffered_bytes += bytes;
    return 1;
}

void lockbox_release(uint32_t id)
{
    if (id < LOCKBOX_CAPACITY)
        table[id].in_use = 0;
}

uint32_t lockbox_active_count(void)
{
    uint32_t n = 0;
    for (int i = 0; i < LOCKBOX_CAPACITY; i++)
        if (table[i].in_use)
            n++;
    return n;
}

uint32_t lockbox_rejected_count(void)
{
    return rejected_count;
}

lockbox_result_t lockbox_listen(uint16_t local_port, uint8_t protocol, uint32_t *out_id)
{
    uint8_t zero_ip = {0, 0, 0, 0};

    return lockbox_claim(local_port, zero_ip, 0, protocol, out_id);
}

uint32_t lockbox_find_listener(uint16_t local_port, uint8_t protocol)
{
    for (uint32_t i = 0; i < LOCKBOX_CAPACITY; i++)
        if (table[i].in_use && table[i].protocol == protocol && table[i].local_port == local_port)
            return i;
    return LOCKBOX_CAPACITY;
}

const char *lockbox_result_string(lockbox_result_t r)
{
    switch (r)
    {
    case LOCKBOX_OK:
        return "OK";
    case LOCKBOX_REJECT_TABLE_FULL:
        return "REJECT (table full)";
    case LOCKBOX_REJECT_IP_QUOTA:
        return "REJECT (per-IP quota)";
    case LOCKBOX_REJECT_ALREADY_EXISTS:
        return "REJECT (already claimed)";
    default:
        return "REJECT (unknown)";
    }
}