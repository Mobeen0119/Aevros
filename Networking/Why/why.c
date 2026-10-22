#include "why.h"
#include "../IPv4/LockBox/lockbox.h"
#include "../../kernel/CPU/irq_guard.h"
#include "../../kernel/Process/task.h"
#include "../../Lib/kprintf.h"

#define WHY_SAY_EVERY_TICKS 100

static uint32_t counts[LOCKBOX_CAPACITY][WHY_COUNT];
static uint32_t last_said[LOCKBOX_CAPACITY][WHY_COUNT];

void why_init(void)
{
    uint32_t f = irq_guard_save();

    for (uint32_t c = 0; c < LOCKBOX_CAPACITY; c++)
        for (int r = 0; r < WHY_COUNT; r++)
        {
            counts[c][r] = 0;
            last_said[c][r] = 0;
        }

    irq_guard_restore(f);
}

int why_note(uint32_t conn_id, why_reason_t reason)
{
    if (conn_id >= LOCKBOX_CAPACITY || reason <= WHY_NONE || reason >= WHY_COUNT)
        return 0;

    uint32_t now = get_ticks();
    uint32_t f = irq_guard_save();

    int say = counts[conn_id][reason] == 0 || now - last_said[conn_id][reason] >= WHY_SAY_EVERY_TICKS;

    counts[conn_id][reason]++;
    if (say)
        last_said[conn_id][reason] = now;

    irq_guard_restore(f);
    return say;
}

uint32_t why_count(uint32_t conn_id, why_reason_t reason)
{
    if (conn_id >= LOCKBOX_CAPACITY || reason <= WHY_NONE || reason >= WHY_COUNT)
        return 0;

    return counts[conn_id][reason];
}

why_reason_t why_top_reason(uint32_t conn_id)
{
    if (conn_id >= LOCKBOX_CAPACITY)
        return WHY_NONE;

    why_reason_t best = WHY_NONE;

    for (int r = WHY_NONE + 1; r < WHY_COUNT; r++)
        if (counts[conn_id][r] > (best == WHY_NONE ? 0 : counts[conn_id][best]))
            best = (why_reason_t)r;

    return best;
}

const char *why_reason_text(why_reason_t reason)
{
    switch (reason)
    {
    case WHY_SEND_WAITING_ACK:
        return "waiting for the peer to acknowledge our last segment before sending more";
    case WHY_CLOSE_WAITING_ACK:
        return "waiting for our data to be acknowledged before closing";
    case WHY_OUT_OF_ORDER:
        return "segments arrived out of order (a packet ahead of it was lost or delayed)";
    case WHY_STALE_RETRANSMIT:
        return "the peer resent data we already had (our ACK was late or lost)";
    case WHY_RETRANSMIT_TIMEOUT:
        return "no ACK arrived in time, so we sent the segment again";
    case WHY_PEER_WINDOW_FULL:
        return "the peer's receive window was full";
    default:
        return "nothing to explain";
    }
}

void why_report(void)
{
    int any = 0;

    kprintf("Why connections did not do more\n");
    kprintf("-------------------------------\n");

    for (uint32_t c = 0; c < LOCKBOX_CAPACITY; c++)
    {
        why_reason_t top = why_top_reason(c);

        if (top == WHY_NONE)
            continue;

        any = 1;
        kprintf("slot %u: mostly %s (%u times)\n", c, why_reason_text(top), counts[c][top]);

        for (int r = WHY_NONE + 1; r < WHY_COUNT; r++)
            if (r != (int)top && counts[c][r] > 0)
                kprintf("         also: %s (%u times)\n", why_reason_text((why_reason_t)r), counts[c][r]);
    }

    if (!any)
        kprintf("nothing to explain: no connection has been held back for any reason\n");
}