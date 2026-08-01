#include "bailiff.h"
#include "../../kernel/Process/task.h"
#include "../../Lib/kprintf.h"
#include "../FrontDesk/frontdesk.h"

static hall_pass_t passes[BAILIFF_MAX_PASSES];
static uint32_t next_pass_id = 1;
static uint32_t denied_total;
static uint32_t transmitted_total;

static uint32_t checksum(const uint8_t *data, uint16_t len)
{
    uint32_t sum = 2166136261u;

    for (uint16_t i = 0; i < len; i++)
    {
        sum ^= data[i];
        sum *= 16777619u;
    }
    return sum;
}

static void reclaim_expired(void)
{
    uint32_t now = get_ticks();

    for (int i = 0; i < BAILIFF_MAX_PASSES; i++)
    {
        if (passes[i].in_use && (now - passes[i].issued_at) > BAILIFF_PASS_TIMEOUT_TICKS)
        {
            kprintf("[Bailiff] pass %u expired unused, reclaiming the slot\n", passes[i].pass_id);
            passes[i].in_use = 0;
        }
    }
}

int bailiff_request_pass(const uint8_t *frame, uint16_t len, uint32_t *out_pass_id)
{
    if (len == 0 || len > BAILIFF_MAX_FRAME)
    {
        kprintf("[Bailiff] refusing to issue a pass for a %u-byte frame ... outside sane bounds\n", len);

        denied_total++;
        return 0;
    }

    reclaim_expired();

    int slot = -1;

    for (int i = 0; i < BAILIFF_MAX_PASSES; i++)
    {
        if (!passes[i].in_use)
        {
            slot = i;
            break;
        }
    }
    if (slot < 0)
    {
        kprintf("[Bailiff] pass table full, refusing to issue another ... deny by default, not unbounded growth\n");
        denied_total++;
        return 0;
    }

    passes[slot].pass_id = next_pass_id++;
    passes[slot].declared_len = len;
    passes[slot].content_checksum = checksum(frame, len);
    passes[slot].issued_at = get_ticks();
    passes[slot].in_use = 1;

    *out_pass_id = passes[slot].pass_id;
    kprintf("[Bailiff] issued pass %u for %u bytes\n", passes[slot].pass_id, len);

    return 1;
}

int bailiff_present_pass(uint32_t pass_id, const uint8_t *frame, uint16_t len)
{
    reclaim_expired();

    for (int i = 0; i < BAILIFF_MAX_PASSES; i++)
    {
        if (!passes[i].in_use || passes[i].pass_id != pass_id)
            continue;

        if (passes[i].declared_len != len)
        {
            kprintf("[Bailiff] pass %u was issued for %u bytes, this is %u ... refusing, not what was authorized\n",
                    pass_id, passes[i].declared_len, len);
            denied_total++;
            return 0;
        }

        if (passes[i].content_checksum != checksum(frame, len))
        {
            kprintf("[Bailiff] pass %u's bytes don't match what was authorized....something changed since the pass was issued, refusing\n", pass_id);
            denied_total++;
            return 0;
        }
        if (!frontdesk_send(frame, len))
        {
            kprintf("[Bailiff] pass %u honored but the driver refused the send\n", pass_id);
            denied_total++;
            passes[i].in_use = 0;
            return 0;
        }

        transmitted_total++;
        kprintf("[Bailiff] pass %u honored, %u bytes handed to FrontDesk\n", pass_id, len);
        passes[i].in_use = 0;
        return 1;
    }
    kprintf("[Bailiff] pass %u doesn't exist or already expire red ... fusing\n", pass_id);
    denied_total++;
    return 0;
}

uint32_t bailiff_denied_count(void)
{
    return denied_total;
}

uint32_t bailiff_transmitted_count(void)
{
    return transmitted_total;
}