#include "ids6.h"

static ids6_hook_fn hooks[IDS6_MAX_HOOKS];

static int hook_count;

int ids6_register_hook(ids6_hook_fn fn)
{
    if (hook_count >= IDS6_MAX_HOOKS)
        return 0;

    hooks[hook_count++] = fn;
    return 1;
}

void ids6_notify(ids6_event_type_t type, const uint8_t ip[16], const char *detail)
{
    static const uint8_t zero_ip[16] = {0};
    const uint8_t *safe_ip = ip ? ip : zero_ip;

    for (int i = 0; i < hook_count; i++)
        hooks[i](type, safe_ip, detail);
}

const char *ids6_event_string(ids6_event_type_t type)
{
    switch (type)
    {
    case IDS6_EVENT_GUESTLIST_DENY:
        return "guestlist6 deny";
    case IDS6_EVENT_CURFEW_REJECT:
        return "curfew6 rate limit";
    case IDS6_EVENT_ROLODEX_DISPUTED:
        return "rolodex6 disputed";
    case IDS6_EVENT_IP_REJECT:
        return "malformed ipv6 packet";
    case IDS6_EVENT_PORT_SCAN_BAN:
        return "sentry6 port scan ban";
    default:
        return "unknown";
    }
}