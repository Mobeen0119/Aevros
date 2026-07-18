#include "mailroom.h"
#include "directory.h"
#include "../../../Lib/kprintf.h"
#include "../../Process/task.h"

#define ETHERTYPE_OFFSET 12
#define ETH_HEADER_SIZE 14
#define MIN_ETHERTYPE 0x0600

#define ETHERTYPE_VLAN 0x8100

extern directory_entry_t __directory_start[];
extern directory_entry_t __directory_end[];

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

    (void)our_mac;
    uint16_t ethertype = (uint16_t)(frame[ETHERTYPE_OFFSET] << 8) | (frame[ETHERTYPE_OFFSET + 1]);

    if (ethertype < MIN_ETHERTYPE || ethertype == ETHERTYPE_VLAN)
    {
        kprintf("[Mailroom] rejected legacy/VLAN-tagged frame (0x%x)\n", ethertype);
        undelivered_count++;
        return MAIL_LEGACY_REJECTED;
    }

    for (directory_entry_t *e = __directory_start; e < __directory_end; e++)
    {
        if (e->ethertype == ethertype)
        {
            kprintf("[Mailroom] delivered to %s, %d bytes\n", e->name, length - 14);
            e->handler(frame + ETH_HEADER_SIZE, (uint16_t)(length - ETH_HEADER_SIZE));
            delivered_count++;
            return MAIL_DELIVERED;
        }
    }
    kprintf("[Mailroom] unknown EtherType 0x%x, no department for it\n", ethertype);
    undelivered_count++;
    return MAIL_UNKNOWN_TYPE;
}
