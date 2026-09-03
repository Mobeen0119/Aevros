#ifndef LEDGER6_H
#define LEDGER6_H

#include <stdint.h>

typedef struct
{
    uint32_t ip_accepted, ip_rejected;
    uint32_t tcp_accepted, tcp_rejected;
    uint32_t udp_accepted, udp_rejected;
    uint32_t icmp_accepted, icmp_rejected;

    uint32_t curfew_denied;
    uint32_t rolodex_entries, rolodex_contradiction;

    uint32_t guestlist_entries;
    uint32_t menu_entries;   // shared with v4, not v6-only
    uint32_t bailiff_denied, bailiff_transmitted; // shared with v4
    uint32_t lockbox_active;
    uint32_t atlas_routes;

    uint32_t scheduler_retransmit, scheduler_gaveup;
    uint32_t sentry_flagged;
    uint32_t fragment_completed, fragment_overlap, fragment_timeout;
} ledger6_snapshot_t;

ledger6_snapshot_t ledger6_snapshot(void);

void ledger6_print(void);

#endif