#include "ledger.h"
#include "../Compass/compass.h"
#include "../Conversation/conversation.h"
#include "../PostCard/postcard.h"
#include "../Echo/echo.h"
#include "../Curfew/curfew.h"
#include "../Roldex/rolodex.h"
#include "../Guestlist/guestlist.h"
#include "../Menu/menu.h"
#include "../Bailiff/bailiff.h"
#include "../LockBox/lockbox.h"
#include "../Scheduler/scheduler.h"
#include "../../Lib/kprintf.h"

ledger_snapshot_t ledger_snapshot(void)
{
    ledger_snapshot_t s;

    s.ip_accepted = compass_accepted_count();
    s.ip_rejected = compass_rejected_count();

    s.tcp_accepted = tcp_accepted_count();
    s.tcp_rejected = tcp_rejected_count();

    s.icmp_accepted = echo_accepted_count();
    s.icmp_rejected = echo_rejected_count();

    s.udp_accepted = postcard_accepted_count();
    s.udp_rejected = postcard_rejected_count();

    s.bailiff_transmitted = bailiff_transmitted_count();
    s.bailiff_denied = bailiff_denied_count();

    s.curfew_denied = curfew_denied_count();

    s.guestlist_entries = guestlist_count();

    s.rolodex_entries = rolodex_count();
    s.rolodex_contradiction = rolodex_contradiction_count();

    s.menu_entries = menu_count();

    s.lockbox_active = lockbox_active_count();

    s.scheduler_retransmit = scheduler_retransmit_count();
    s.scheduler_gaveup = scheduler_giveup_count();

    return s;
}

void ledger_print(void)
{
    ledger_snapshot_t s = ledger_snapshot();

    kprintf("\n");
    kprintf("=============================================================\n");
    kprintf("                 AEVROS NETWORK LEDGER\n");
    kprintf("=============================================================\n");

    kprintf("\n[FrontDesk / Compass]\n");
    kprintf("  IPv4 Packets Accepted .......... %u\n", s.ip_accepted);
    kprintf("  IPv4 Packets Rejected .......... %u\n", s.ip_rejected);

    kprintf("\n[Conversation (TCP)]\n");
    kprintf("  Accepted Connections ........... %u\n", s.tcp_accepted);
    kprintf("  Rejected Segments .............. %u\n", s.tcp_rejected);

    kprintf("\n[Postcard (UDP)]\n");
    kprintf("  Accepted Datagrams ............. %u\n", s.udp_accepted);
    kprintf("  Rejected Datagrams ............. %u\n", s.udp_rejected);

    kprintf("\n[Echo (ICMP)]\n");
    kprintf("  Accepted Echo Requests ......... %u\n", s.icmp_accepted);
    kprintf("  Rejected Echo Requests ......... %u\n", s.icmp_rejected);

    kprintf("\n---------------- Security Office ----------------\n");
    kprintf("  Curfew Blocks .................. %u\n", s.curfew_denied);
    kprintf("  Rolodex Entries ................ %u\n", s.rolodex_entries);
    kprintf("  Rolodex Conflicts .............. %u\n", s.rolodex_contradiction);
    kprintf("  GuestList Rules ................ %u\n", s.guestlist_entries);
    kprintf("  Menu Open Ports ................ %u\n", s.menu_entries);

    kprintf("\n---------------- Transport ----------------------\n");
    kprintf("  LockBox Active Slots ........... %u\n", s.lockbox_active);

    kprintf("\n---------------- Outbound -----------------------\n");
    kprintf("  Bailiff Frames Authorized ...... %u\n", s.bailiff_transmitted);
    kprintf("  Bailiff Frames Refused ......... %u\n", s.bailiff_denied);

    kprintf("\n---------------- Reliability ---------------------\n");
    kprintf("  Scheduler Retransmits .......... %u\n", s.scheduler_retransmit);
    kprintf("  Scheduler Gave-Up Connections ... %u\n", s.scheduler_gaveup);

    kprintf("=============================================================\n");
}