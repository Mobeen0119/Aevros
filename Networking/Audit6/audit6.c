#include "audit6.h"
#include "../IDS6/ids6.h"
#include "../../kernel/Process/task.h"
#include "../../Lib/string.h"
#include "../../Lib/kprintf.h"
static audit6_record_t log_buf[AUDIT6_LOG_CAPACITY];
static uint32_t write_pos;

static uint32_t total_logged;

static void print_ip6(const uint8_t ip[16])
{
    for (int i = 0; i < 16; i += 2)
    {
        kprintf("%02x%02x", ip[i], ip[i + 1]);
        if (i < 14)
            kprintf(":");
    }
}

static void on_event(ids6_event_type_t type, const uint8_t ip[16], const char *detail)
{
    audit6_record_t *r = &log_buf[write_pos];

    r->tick = get_ticks();
    r->event_type = (uint8_t)type;
    memcpy(r->ip, ip, 16);

    write_pos = (write_pos + 1) % AUDIT6_LOG_CAPACITY;
    total_logged++;

    kprintf("[Audit6] tick %u: %s (", r->tick, ids6_event_string(type));
    print_ip6(ip);
    kprintf(")%s%s\n", detail ? " - " : "", detail ? detail : "");
}

void audit6_start(void)
{
    ids6_register_hook(on_event);
}

uint32_t audit6_count(void)
{
    return total_logged;
}

int audit6_get(uint32_t i, audit6_record_t *out)
{
    uint32_t held = total_logged < AUDIT6_LOG_CAPACITY ? total_logged : AUDIT6_LOG_CAPACITY;

    if (i >= held)
    {
        memset(out, 0, sizeof(*out));
        return 0;
    }
    uint32_t oldest = total_logged < AUDIT6_LOG_CAPACITY ? 0 : write_pos;
    uint32_t idx = (oldest + i) % AUDIT6_LOG_CAPACITY;

    *out = log_buf[idx];
    return 1;
}

void audit6_dump(void)
{
    uint32_t held = total_logged < AUDIT6_LOG_CAPACITY ? total_logged : AUDIT6_LOG_CAPACITY;
    kprintf("[Audit6] %u events logged total, showing last %u:\n", total_logged, held);

    for (uint32_t i = 0; i < held; i++)
    {
        audit6_record_t r;
        audit6_get(i, &r);

        kprintf("  [%u] tick %u: %s (", i, r.tick, ids6_event_string((ids6_event_type_t)r.event_type));
        print_ip6(r.ip);
        kprintf(")\n");
    }
}