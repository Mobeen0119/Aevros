#include "mailroom.h"
#include "../../../Lib/kprintf.h"
#include "../../Process/task.h"

#define ETHERTYPE_OFFSET 12
#define ETH_HEADER_SIZE 14
#define MIN_ETHERTYPE 0x0600

#define ETHERTYPE_VLAN 0x8100

static uint32_t delivered_count;
static uint32_t undelivered_count;

uint32_t mailroom_log_count(void)
{
    return delivered_count + undelivered_count;
}

uint32_t mailroom_undelivered_count(void)
{
    return undelivered_count;
}

mail_result_t mailroom_deliver(const uint8_t *frame, uint16_t length, const uint8_t our_mac[6])
{

    uint16_t ethertype = (uint16_t)(frame[ETHERTYPE_OFFSET] << 8) | (frame[ETHERTYPE_OFFSET + 1]);

    if (ethertype < MIN_ETHERTYPE)
    {
        kprintf("[Mailroom] rejected legacy length-field frame (value %d)\n", ethertype);
        undelivered_count++;
        return MAIL_LEGACY_REJECTED;
    }

    if (ethertype == ETHERTYPE_VLAN)
    {
        kprintf("[Mailroom] rejected VLAN-tagged frame, not supported\n");

        undelivered_count++;
        return MAIL_LEGACY_REJECTED;
    }

    switch (ethertype)
    {
    case ETHERTYPE_ARP:
        kprintf("[Mailroom] delivered to Rolodex (ARP), %d bytes\n", length - 14);
        delivered_count++;
        return MAIL_DELIVERED;
    case ETHERTYPE_IPV4:
        kprintf("[Mailroom] delivered to Compass (IPv4), %d bytes\n", length - 14);
        delivered_count++;
        return MAIL_DELIVERED;

    default:
        kprintf("[Mailroom] unknown EtherType 0x%x, no department for it\n", ethertype);

        undelivered_count++;
        return MAIL_UNKNOWN_TYPE;
    }
}
