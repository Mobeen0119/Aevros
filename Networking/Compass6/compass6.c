#include "compass6.h"
#include "ip6_directory.h"
#include "../mailroom/directory.h"
#include "../Rolodex6/rolodex6.h"
#include "../GuestList6/guestlist6.h"
#include "../Curfew6/curfew6.h"
#include "../../Lib/kprintf.h"

#define MIN_IP6_HEADER 40
#define NEXT_HEADER_FRAGMENT 44

extern ip6_directory_entry_t __ip6_directory_start[];
extern ip6_directory_entry_t __ip6_directory_end[];

static uint32_t accepted, rejected;

static ip6_verdict_t compass6_check(const uint8_t *payload, uint16_t length)
{
    if (length < MIN_IP6_HEADER)
        return IP6_REJECT_TOO_SHORT;

    uint8_t version = (uint8_t)(payload[0] >> 4);
    if (version != 6)
        return IP6_REJECT_BAD_VERSION;

    uint16_t payload_len = (uint16_t)((payload[4] << 8) | payload[5]);
    if ((uint32_t)MIN_IP6_HEADER + payload_len > length)
        return IP6_REJECT_LENGTH_MISMATCH;

    uint8_t next_header = payload[6];
    if (next_header == NEXT_HEADER_FRAGMENT)
        return IP6_REJECT_FRAGMENTED;

    return IP6_ACCEPT;
}

void compass6_handle(const uint8_t *payload, uint16_t length, const uint8_t src_mac[6])
{
    ip6_verdict_t v = compass6_check(payload, length);

    if (v != IP6_ACCEPT)
    {
        kprintf("[Compass6] rejected: %s\n", ip6_verdict_string(v));
        rejected++;
        return;
    }

    accepted++;

    uint8_t next_header = payload[6];
    const uint8_t *src_ip = payload + 8;
    const uint8_t *dst_ip = payload + 24;
    uint16_t payload_len = (uint16_t)((payload[4] << 8) | payload[5]);

   if (guestlist6_check(src_ip) == GUESTLIST6_DENIED)
    {
        kprintf("[Compass6] source is on the Guestlist6 deny list, refusing outright\n");
        rejected++;
        return;
    }

    int guestlist_allow = (guestlist6_check(src_ip) == GUESTLIST6_ALLOWED);

    if (!guestlist_allow && !curfew6_check(src_ip))
    {
        kprintf("[Compass6] source tripped Curfew6's rate limit, refusing\n");
        rejected++;
        return;
    }

    rolodex6_learn(src_ip, src_mac);

    kprintf("[Compass6] accepted, next_header %d, payload %u bytes\n", next_header, payload_len);

    for (ip6_directory_entry_t *e = __ip6_directory_start; e < __ip6_directory_end; e++)
    {
        if (e->next_header == next_header)
        {
            e->handler(payload + MIN_IP6_HEADER, payload_len, src_ip, dst_ip);
            return;
        }
    }

    kprintf("[Compass6] no handler registered for next_header %d\n", next_header);
}

uint32_t compass6_accepted_count(void)
{
    return accepted;
}

uint32_t compass6_rejected_count(void)
{
    return rejected;
}

const char *ip6_verdict_string(ip6_verdict_t v)
{
    switch (v)
    {
    case IP6_ACCEPT:
        return "ACCEPT";
    case IP6_REJECT_TOO_SHORT:
        return "REJECT (too short)";
    case IP6_REJECT_BAD_VERSION:
        return "REJECT (not IPv6)";
    case IP6_REJECT_LENGTH_MISMATCH:
        return "REJECT (length lied)";
    case IP6_REJECT_FRAGMENTED:
        return "REJECT (fragmented, not supported)";
    default:
        return "REJECT (unknown)";
    }
}

DIRECTORY_ENTRY(0x86DD, compass6_handle, "Compass6 (IPv6)");