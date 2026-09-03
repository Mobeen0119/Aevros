#include "ledger6.h"
#include "../Compass6/compass6.h"
#include "../Conversation6/conversation6.h"
#include "../PostCard6/postcard6.h"
#include "../Echo6/echo6.h"
#include "../Curfew6/curfew6.h"
#include "../Rolodex6/rolodex6.h"
#include "../GuestList6/guestlist6.h"
#include "../Menu/menu.h"
#include "../Bailiff/bailiff.h"
#include "../LockBox6/lockbox6.h"
#include "../Atlas6/atlas6.h"
#include "../Scheduler6/scheduler6.h"
#include "../Sentry6/sentry6.h"
#include "../Fragment6/fragment6.h"
#include "../../Lib/kprintf.h"

ledger6_snapshot_t ledger6_snapshot(void)
{
    ledger6_snapshot_t s;

    s.ip_accepted = compass6_accepted_count();
    s.ip_rejected = compass6_rejected_count();

    s.tcp_accepted = tcp6_accepted_count();
    s.tcp_rejected = tcp6_rejected_count();

    s.udp_accepted = postcard6_accepted_count();
    s.udp_rejected = postcard6_rejected_count();

    s.icmp_accepted = echo6_accepted_count();
    s.icmp_rejected = echo6_rejected_count();

    s.curfew_denied = curfew6_denied_count();
    s.rolodex_entries = rolodex6_entry_count();
    s.rolodex_contradiction = rolodex6_contradiction_count();

    s.guestlist_entries = guestlist6_count();
    s.menu_entries = menu_count();

    s.bailiff_transmitted = bailiff_transmitted_count();
    s.bailiff_denied = bailiff_denied_count();

    s.lockbox_active = lockbox6_active_count();
    s.atlas_routes = atlas6_route_count();

    s.scheduler_retransmit = scheduler6_retransmit_count();
    s.scheduler_gaveup = scheduler6_giveup_count();

    s.sentry_flagged = sentry6_flagged_count();

    s.fragment_completed = fragment6_completed_count();
    s.fragment_overlap = fragment6_overlap_count();
    s.fragment_timeout = fragment6_timeout_count();

    return s;
}

void ledger6_print(void)
{
    ledger6_snapshot_t s = ledger6_snapshot();

    kprintf("\n");
    kprintf("=============================================================\n");
    kprintf("                 AEVROS IPv6 NETWORK LEDGER\n");
    kprintf("=============================================================\n");

    kprintf("\n[Compass6 / IP]\n");
    kprintf("  IPv6 Packets Accepted .......... %u\n", s.ip_accepted);
    kprintf("  IPv6 Packets Rejected .......... %u\n", s.ip_rejected);

    kprintf("\n[Conversation6 (TCP)]\n");
    kprintf("  Accepted Connections ........... %u\n", s.tcp_accepted);
    kprintf("  Rejected Segments .............. %u\n", s.tcp_rejected);

    kprintf("\n[Postcard6 (UDP)]\n");
    kprintf("  Accepted Datagrams ............. %u\n", s.udp_accepted);
    kprintf("  Rejected Datagrams ............. %u\n", s.udp_rejected);

    kprintf("\n[Echo6 (ICMPv6)]\n");
    kprintf("  Accepted Messages ............... %u\n", s.icmp_accepted);
    kprintf("  Rejected Messages ............... %u\n", s.icmp_rejected);

    kprintf("\n---------------- Security Office ----------------\n");
    kprintf("  Curfew6 Blocks .................. %u\n", s.curfew_denied);
    kprintf("  Rolodex6 Entries ................ %u\n", s.rolodex_entries);
    kprintf("  Rolodex6 Contradictions ......... %u\n", s.rolodex_contradiction);
    kprintf("  GuestList6 Rules ................ %u\n", s.guestlist_entries);
    kprintf("  Sentry6 Port-Scan Bans ........... %u\n", s.sentry_flagged);
    kprintf("  Menu Open Ports (shared v4+v6) ... %u\n", s.menu_entries);

    kprintf("\n---------------- Transport ----------------------\n");
    kprintf("  LockBox6 Active Slots ........... %u\n", s.lockbox_active);
    kprintf("  Atlas6 Routes .................... %u\n", s.atlas_routes);

    kprintf("\n---------------- Outbound (shared v4+v6) ---------\n");
    kprintf("  Bailiff Frames Authorized ....... %u\n", s.bailiff_transmitted);
    kprintf("  Bailiff Frames Refused ........... %u\n", s.bailiff_denied);

    kprintf("\n---------------- Reliability ---------------------\n");
    kprintf("  Scheduler6 Retransmits ........... %u\n", s.scheduler_retransmit);
    kprintf("  Scheduler6 Gave-Up Connections .... %u\n", s.scheduler_gaveup);

    kprintf("\n---------------- Fragmentation --------------------\n");
    kprintf("  Reassemblies Completed ........... %u\n", s.fragment_completed);
    kprintf("  Overlap Attacks Rejected ......... %u\n", s.fragment_overlap);
    kprintf("  Reassemblies Timed Out ............ %u\n", s.fragment_timeout);

    kprintf("=============================================================\n");
}