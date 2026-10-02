
#include "net_trace.h"

static nettrace_event_t events[NETTRACE_MAX_EVENTS];
static uint8_t claimed[NETTRACE_MAX_EVENTS];
static uint32_t event_count = 0;

static uint8_t is_reverse_match(const netwatch_event_t *a, const netwatch_event_t *b)
{
    if (a->protocol != b->protocol)
        return 0;

    if (a->direction == b->direction)
        return 0;

    if (a->src_ip != b->dst_ip || a->dst_ip != b->src_ip)
        return 0;

    if (a->src_port != b->dst_port || a->dst_port != b->src_port)
        return 0;

    return 1;
}

void nettrace_init(void)
{
    nettrace_clear();
}

void nettrace_rebuild_at(const netwatch_snapshot_t *snapshot, uint64_t now)
{
    if (!snapshot)
        return;

    nettrace_clear();

    event_count = snapshot->event_count;

    if (event_count > NETTRACE_MAX_EVENTS)
        event_count = NETTRACE_MAX_EVENTS;

    for (uint32_t i = 0; i < event_count; i++)
    {
        const netwatch_event_t *source = &snapshot->events[i];

        if (!source->valid)
        {
            event_count = i;
            break;
        }

        events[i].event_id = source->id;
        events[i].previous_id = i > 0 ? snapshot->events[i - 1].id : 0;
        events[i].related_id = 0;

        events[i].protocol = source->protocol;

        events[i].direction = source->direction;
        events[i].length = source->length;

        events[i].src_ip = source->src_ip;
        events[i].tick = source->tick;

        events[i].status = NETTRACE_STATUS_NONE;

        events[i].dst_ip = source->dst_ip;
        events[i].src_port = source->src_port;

        events[i].dst_port = source->dst_port;
        events[i].relation = NETTRACE_RELATION_NONE;
        events[i].valid = 1;

        for (uint32_t k = 0; k < i; k++)
        {
            if (claimed[k] || events[k].relation == NETTRACE_RELATION_RESPONSE)
                continue;

            if (is_reverse_match(source, &snapshot->events[k]))
            {
                events[i].related_id = snapshot->events[k].id;

                events[i].relation = NETTRACE_RELATION_RESPONSE;
                claimed[k] = 1;
                break;
            }
        }
    }

    for (uint32_t i = 0; i < event_count; i++)
    {
        if (events[i].direction != NETWATCH_TX)
            continue;

        if (claimed[i])
            events[i].status = NETTRACE_STATUS_ANSWERED;
        else if (now - events[i].tick > NETTRACE_TIMEOUT_TICKS)
            events[i].status = NETTRACE_STATUS_TIMED_OUT;
        else
            events[i].status = NETTRACE_STATUS_PENDING;
    }
}

const nettrace_event_t *nettrace_get_event(uint32_t index)
{
    if (index >= event_count)
        return 0;

    if (!events[index].valid)
        return 0;

    return &events[index];
}

uint32_t nettrace_get_event_count(void)
{
    return event_count;
}

void nettrace_get_snapshot(nettrace_snapshot_t *snapshot)
{
    if (!snapshot)
        return;

    snapshot->event_count = event_count;

    for (uint32_t i = 0; i < event_count; i++)
        snapshot->events[i] = events[i];

    for (uint32_t i = event_count; i < NETTRACE_MAX_EVENTS; i++)
        snapshot->events[i].valid = 0;
}

int32_t nettrace_find_event(uint32_t event_id)
{
    for (uint32_t i = 0; i < event_count; i++)
    {
        if (events[i].valid && events[i].event_id == event_id)
            return (int32_t)i;
    }

    return -1;
}

int32_t nettrace_find_related(uint32_t event_id)
{
    int32_t index = nettrace_find_event(event_id);

    if (index < 0)
        return -1;

    uint32_t related_id = events[index].related_id;

    if (related_id)
        return nettrace_find_event(related_id);

    for (uint32_t i = 0; i < event_count; i++)
    {
        if (events[i].valid && events[i].related_id == event_id)
            return (int32_t)i;
    }

    return -1;
}

void nettrace_clear(void)
{
    for (uint32_t i = 0; i < NETTRACE_MAX_EVENTS; i++)
    {
        events[i].event_id = 0;
        events[i].previous_id = 0;
        events[i].related_id = 0;

        events[i].protocol = NETWATCH_PROTO_UNKNOWN;
        events[i].direction = NETWATCH_RX;

        events[i].length = 0;

        events[i].src_ip = 0;
        events[i].dst_ip = 0;

        events[i].src_port = 0;
        events[i].dst_port = 0;

        events[i].relation = NETTRACE_RELATION_NONE;
        events[i].valid = 0;
        claimed[i] = 0;
    }

    event_count = 0;
}