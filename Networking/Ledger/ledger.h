#ifndef LEDGER_H
#define LEDGER_H

#include <stdint.h>

typedef struct
{
    uint32_t ip_accepted, ip_rejected;
    uint32_t tcp_accepted, tcp_rejected;
    uint32_t udp_accepted, udp_rejected;
    uint32_t icmp_accepted, icmp_rejected;

    uint32_t curfew_denied;
    uint32_t rolodex_contradiction, rolodex_entries;

    uint32_t guestlist_entries;
    uint32_t menu_entries;
    uint32_t bailiff_denied, bailiff_transmitted;
    uint32_t lockbox_active;
} ledger_snapshot_t;

ledger_snapshot_t ledger_snapshot(void);

void ledger_print(void);

#endif