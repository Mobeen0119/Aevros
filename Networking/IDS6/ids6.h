#ifndef IDS6_H
#define IDS6_H
#include <stdint.h>

#define IDS6_MAX_HOOKS 8

typedef enum
{
    IDS6_EVENT_GUESTLIST_DENY = 1,
    IDS6_EVENT_CURFEW_REJECT,
    IDS6_EVENT_ROLODEX_DISPUTED,
    IDS6_EVENT_IP_REJECT,
    IDS6_EVENT_PORT_SCAN_BAN
} ids6_event_type_t;

typedef void (*ids6_hook_fn)(ids6_event_type_t type, const uint8_t ip[16], const char *detail);

int ids6_register_hook(ids6_hook_fn fn);

void ids6_notify(ids6_event_type_t type, const uint8_t ip[16], const char *detail);

const char *ids6_event_string(ids6_event_type_t type);

#endif