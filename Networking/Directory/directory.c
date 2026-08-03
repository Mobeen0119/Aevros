#include "directory.h"
#include "../SwitchBoard/switchboard.h"
#include "../Menu/menu.h"
#include "../LockBox/lockbox.h"
#include "../../kernel/Process/task.h"
#include "../../Lib/string.h"
#include "../../Lib/kprintf.h"

#define DNS_MAX_PACKET 320

typedef struct
{
    char hostname[DIRECTORY_MAX_HOSTNAME + 1];
    uint8_t ip[4];
    uint32_t expired_at_tick;
    int in_use;

} directory_entry_t;

static directory_entry_t cache[DIRECTORY_CACHE_CAPACITY];
static uint8_t dns_server[4];
static int have_server;
          
static char pending_hostname[DIRECTORY_MAX_HOSTNAME + 1];
static uint16_t pending_id;
static uint32_t pending_sent_tick;
static int pending_active;
static int pending_retried;
static uint16_t next_id = 1;

static int find_cache_slot(const char *hostname)
{

    for (int i = 0; i < DIRECTORY_CACHE_CAPACITY; i++)
        if (cache[i].in_use && (strncmp(cache[i].hostname, hostname, DIRECTORY_MAX_HOSTNAME) == 0))
            return i;

    return -1;
}

static void store_in_cache(const char *hostname, const uint8_t ip[4], uint32_t ttl_seconds)
{
    int slot = find_cache_slot(hostname);

    if (slot < 0)
        for (int i = 0; i < DIRECTORY_CACHE_CAPACITY; i++)
            if (!cache[i].in_use)
            {
                slot = i;
                break;
            }

    if (slot < 0)
        slot = 0;

    strncpy(cache[slot].hostname, hostname, DIRECTORY_MAX_HOSTNAME);
    cache[slot].hostname[DIRECTORY_MAX_HOSTNAME] = 0;

    memcpy(cache[slot].ip, ip, 4);
    cache[slot].expired_at_tick = get_ticks() + ttl_seconds * 100;
    cache[slot].in_use = 1;
}

static uint16_t encode_name(uint8_t *out, const char *hostname)
{
    uint16_t out_i = 0, label_start = 0, i = 0;

    for (;;)
    {
        char c = hostname[i];

        if (c == '.' || c == 0)
        {
            uint8_t label_len = (uint8_t)(i - label_start);
            out[out_i++] = label_len;

            memcpy(out + out_i, hostname + label_start, label_len);

            out_i = (uint16_t)(out_i + label_len);

            label_start = (uint16_t)(i + 1);

            if (c == 0)
                break;
        }
        i++;
    }
    out[out_i++] = 0;
    return out_i;
}

static uint16_t skip_name(const uint8_t *packet, uint16_t offset)
{
    while (packet[offset] != 0)
    {
        if ((packet[offset] & 0xC0) == 0xC0)
            return (uint16_t)(offset + 2);

        offset = (uint16_t)(offset + packet[offset] + 1);
    }

    return (uint16_t)(offset + 1);
}

void directory_start(void)
{

    menu_open_port(DIRECTORY_CLIENT_PORT, 17);
    uint32_t id;

    lockbox_listen(DIRECTORY_CLIENT_PORT, 17, &id);
}

void directory_set_server(const uint8_t dns_ip[4])
{
    memcpy(dns_server, dns_ip, 4);

    have_server = (dns_ip[0] || dns_ip[1] || dns_ip[2] || dns_ip[3]);
}

int directory_lookup_cached(const char *hostname, uint8_t out_ip[4])
{

    int slot = find_cache_slot(hostname);

    if (slot < 0)
        return 0;

    if (get_ticks() >= cache[slot].expired_at_tick)
    {
        cache[slot].in_use = 0;
        kprintf("[Directory] %s's cache entry expired\n", hostname);
        return 0;
    }
    memcpy(out_ip, cache[slot].ip, 4);

    return 1;
}

int directory_query(const char *hostname, const uint8_t our_mac[6], const uint8_t our_ip[4])
{

    uint8_t cached[4];

    if (directory_lookup_cached(hostname, cached))
        return 1;

    if (!have_server)
    {
        kprintf("[Directory] no DNS server configured, can't resolve %s\n", hostname);
        return 0;
    }

    if (pending_active)
    {
        kprintf("[Directory] already resolving something else, refusing to start %s\n", hostname);
        return 0;
    }

    uint8_t packet[DNS_MAX_PACKET];

    pending_id = next_id++;

    packet[0] = (uint8_t)(pending_id >> 8);
    packet[1] = (uint8_t)pending_id;
    packet[2] = 0x01;
    packet[3] = 0x00;

    packet[4] = 0;
    packet[5] = 1;
    packet[6] = 0;

    packet[7] = 0;
    packet[8] = 0;
    packet[9] = 0;

    packet[10] = 0;
    packet[11] = 0;

    uint16_t i = 12;

    i = (uint16_t)(i + encode_name(packet + i, hostname));
    packet[i++] = 0;
    packet[i++] = 1;
    packet[i++] = 0;
    packet[i++] = 1;

    switchboard_send_udp(DIRECTORY_CLIENT_PORT, dns_server, DIRECTORY_SERVER_PORT, our_mac, our_ip, packet, i);

    strncpy(pending_hostname, hostname, DIRECTORY_MAX_HOSTNAME);

    pending_hostname[DIRECTORY_MAX_HOSTNAME] = 0;

    pending_sent_tick = get_ticks();

    pending_active = 1;
    pending_retried = 0;

    kprintf("[Directory] querying for %s (id %u)\n", hostname, pending_id);
    return 1;
}

void directory_tick(const uint8_t our_mac[6], const uint8_t our_ip[4])
{
    if (!pending_active)
        return;

    uint8_t src_ip[4];
    uint16_t src_port;

    uint8_t packet[DNS_MAX_PACKET];

    uint16_t got = switchboard_recv_udp(DIRECTORY_CLIENT_PORT, src_ip, &src_port, packet, sizeof(packet));

    if (got >= 12)
    {

        uint16_t reply_id = (uint16_t)((packet[0] << 8) | packet[1]);
        uint16_t ancount = (uint16_t)((packet[6] << 8) | packet[7]);

        if (reply_id == pending_id && ancount > 0)
        {
            uint16_t offset = skip_name(packet, 12);
            offset = (uint16_t)(offset + 4);

            for (uint16_t rec = 0; rec < ancount && offset + 10 <= got; rec++)
            {

                offset = skip_name(packet, offset);

                uint16_t rtype = (uint16_t)((packet[offset] << 8) | packet[offset + 1]);
                uint16_t rclass = (uint16_t)((packet[offset + 2] << 8) | packet[offset + 3]);

                uint32_t ttl = (uint32_t)((packet[offset + 4] << 24) | (packet[offset + 5] << 16) |
                                          (packet[offset + 6] << 8) | packet[offset + 7]);

                uint16_t rdlength = (uint16_t)((packet[offset + 8] << 8) | packet[offset + 9]);
                offset = (uint16_t)(offset + 10);

                if (rtype == 1 && rclass == 1 && rdlength == 4 && offset + 4 <= got)
                {
                    store_in_cache(pending_hostname, packet + offset, ttl == 0 ? 60 : ttl);

                    kprintf("[Directory] %s resolved to %d.%d.%d.%d\n", pending_hostname,
                            packet[offset], packet[offset + 1], packet[offset + 2], packet[offset + 3]);

                    pending_active = 0;
                    return;
                }

                offset = (uint16_t)(offset + rdlength);
            }

            kprintf("[Directory] reply for %s had no usable A record\n", pending_hostname);

            pending_active = 0;
            return;
        }
    }

    if (get_ticks() - pending_sent_tick >= DIRECTORY_TIMEOUT_TICKS)
    {
        if (!pending_retried)
        {

            kprintf("[Directory] no reply for %s, retrying once\n", pending_hostname);
            pending_sent_tick = get_ticks();

            char hostname_copy[DIRECTORY_MAX_HOSTNAME + 1];

            strncpy(hostname_copy, pending_hostname, DIRECTORY_MAX_HOSTNAME);

            hostname_copy[DIRECTORY_MAX_HOSTNAME] = 0;

            pending_active = 0;

            directory_query(hostname_copy, our_mac, our_ip);

            pending_retried = 1;
        }
        else
        {
            kprintf("[Directory] giving up on %s", pending_hostname);

            pending_active = 0;
        }
    }
}
