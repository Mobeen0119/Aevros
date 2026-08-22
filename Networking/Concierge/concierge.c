#include "concierge.h"
#include "../Conversation/rapport.h"
#include "../Conversation6/rapport6.h"
#include "../Scheduler/scheduler.h"
#include "../Scheduler6/scheduler6.h"
#include "../Foyer/foyer.h"
#include "../GuestList/guestlist.h"
#include "../GuestList6/guestlist6.h"
#include "../Landlord/landlord.h"
#include "../Directory/directory.h"
#include "../Rolodex6/rolodex6.h"
#include "../Foyer6/foyer6.h"
#include "../FrontDoor/frontdoor.h"
#include "../Rolodex/rolodex.h"

#define CONCIERGE_TICK_DIVISOR 10

void concierge_tick(const uint8_t our_mac[6])
{
    rapport_tick();
    scheduler_tick();
    foyer_tick(our_mac);
    guestlist_tick();
    landlord_tick(our_mac);

    static int dns_server_applied;
    if (!dns_server_applied && landlord_get_state() == LANDLORD_BOUND)
    {
        uint8_t dns_ip[4];
        landlord_get_dns_server(dns_ip);
        directory_set_server(dns_ip);
        dns_server_applied = 1;
    }

    uint8_t our_ip[4];
    rolodex_get_ip(our_ip);
    directory_tick(our_mac, our_ip);

    frontdoor_tick();

    rapport6_tick();
    scheduler6_tick();
    rolodex6_tick();
    guestlist6_tick();

    uint8_t our_ip6[16];
    rolodex6_get_ip(our_ip6);
    foyer6_tick(our_mac, our_ip6);
}

void concierge_maybe_tick(const uint8_t our_mac[6])
{
    static uint32_t counter;

    counter++;
    if (counter % CONCIERGE_TICK_DIVISOR != 0)
        return;

    concierge_tick(our_mac);
}