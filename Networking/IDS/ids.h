#ifndef IDS_H
#define IDS_H
#include <stdint.h>

#define IDS_MAX_HOOKS 8

typedef enum
{
    IDS_EVENT_GUESTLIST_DENY = 1,
    IDS_EVENT_CURFEW_REJECT,

    IDS_EVENT_ROLODEX_DISPUTED,
    IDS_EVENT_IP_REJECT,

    IDS_EVENT_WATCHLIST_FLAG,
    IDS_EVENT_PORT_SCAN_BAN
} ids_event_type_t;

typedef void (*ids_hook_fn)(ids_event_type_t type, const uint8_t ip[4], const char *detail);

int ids_register_hook(ids_hook_fn fn);

void ids_notify(ids_event_type_t type, const uint8_t ip[4], const char *detail);

const char *ids_event_string(ids_event_type_t type);

#endif