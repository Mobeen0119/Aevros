#include "audit.h"
#include "../IDS/ids.h"
#include "../../kernel/Process/task.h"
#include "../../Lib/string.h"
#include "../../Lib/kprintf.h"

static audit_record_t log_buf[AUDIT_LOG_CAPACITY];
static uint32_t write_pos;
static uint32_t total_logged;

static void on_event(ids_event_type_t type, const uint8_t ip[4], const char *detail)
{
    audit_record_t *r = &log_buf[write_pos];

    r->tick = get_ticks();

    r->event_type = (uint8_t)type;

    memcpy(r->ip, ip, 4);

    write_pos = (write_pos + 1) % AUDIT_LOG_CAPACITY;

    total_logged++;

    kprintf("[Audit] tick %u: %s (%d.%d.%d.%d)%s%s\n",
            r->tick, ids_event_string(type), ip[0], ip[1], ip[2], ip[3],
            detail ? " - " : "", detail ? detail : "");
}

void audit_start(void)
{
    ids_register_hook(on_event);
}

uint32_t audit_count(void)
{
    return total_logged;
}

int audit_get(uint32_t i, audit_record_t *out)
{
    uint32_t held = total_logged < AUDIT_LOG_CAPACITY ? total_logged : AUDIT_LOG_CAPACITY;

    if (i >= held)
    {
        memset(out, 0, sizeof(*out));
        return 0;
    }
    uint32_t oldest = total_logged < AUDIT_LOG_CAPACITY ? 0 : write_pos;

    uint32_t idx = (oldest + i) % AUDIT_LOG_CAPACITY;

    *out = log_buf[idx];

    return 1;
}

void audit_dump(void)
{
    uint32_t held = total_logged < AUDIT_LOG_CAPACITY ? total_logged : AUDIT_LOG_CAPACITY;
    kprintf("[Audit] %u events logged total, showing last %u:\n", total_logged, held);

    for (uint32_t i = 0; i < held; i++)
    {
        audit_record_t r;
        audit_get(i, &r);

        kprintf("  [%u] tick %u: %s (%d.%d.%d.%d)\n",
                i, r.tick, ids_event_string((ids_event_type_t)r.event_type), r.ip[0], r.ip[1], r.ip[2], r.ip[3]);
    }
}
