#include "ids.h"

static ids_hook_fn hooks[IDS_MAX_HOOKS];
static int hook_count;

int ids_register_hook(ids_hook_fn fn)
{
    if (hook_count >= IDS_MAX_HOOKS)
        return 0;

    hooks[hook_count++] = fn;

    return 1;
}

void ids_notify(ids_event_type_t type, const uint8_t ip[4], const char *detail)
{
    static const uint8_t zero_ip[4] = {0, 0, 0, 0};

    const uint8_t *safe_ip = ip ? ip : zero_ip;

    for (int i = 0; i < hook_count; i++)
        hooks[i](type, safe_ip, detail);
}

const char *ids_event_string(ids_event_type_t type)
{
    switch (type)
    {
    case IDS_EVENT_GUESTLIST_DENY:
        return "guestlist deny";

    case IDS_EVENT_CURFEW_REJECT:
        return "curfew rate limit";

    case IDS_EVENT_ROLODEX_DISPUTED:
        return "rolodex disputed";

    case IDS_EVENT_IP_REJECT:
        return "malformed ip packet";

    case IDS_EVENT_WATCHLIST_FLAG:
        return "watchlist flood flag";

    case IDS_EVENT_PORT_SCAN_BAN:
        return "sentry port scan ban";

    default:
        return "unknown";
    }
}
