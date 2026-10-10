#include "provenance.h"
#include "../kernel/CPU/irq_guard.h"

uint32_t get_ticks(void);

static provenance_record_t records[PROVENANCE_MAX_RECORDS];

static uint32_t record_count;
static uint32_t next_id;
static uint32_t dropped;

static void reset_locked(void)
{
    record_count = 0;
    next_id = 1;
    dropped = 0;

    for (uint32_t i = 0; i < PROVENANCE_MAX_RECORDS; i++)
        records[i].valid = 0;
}

static uint32_t oldest_slot(void)
{
    if (record_count < PROVENANCE_MAX_RECORDS)
        return 0;
    return (next_id - 1) % PROVENANCE_MAX_RECORDS;
}

static int entity_equal(provenance_entity_ref_t a, provenance_entity_ref_t b)
{
    return a.type == b.type && a.id == b.id;
}

void provenance_init(void)
{
    uint32_t f = irq_guard_save();
    reset_locked();
    irq_guard_restore(f);
}

void provenance_clear(void)
{
    provenance_init();
}

uint32_t provenance_record(provenance_entity_ref_t source, provenance_entity_ref_t target, provenance_relation_t relation, provenance_reason_t reason)
{
    if (source.type == PROVENANCE_ENTITY_NONE || target.type == PROVENANCE_ENTITY_NONE || relation == PROVENANCE_RELATION_NONE)
        return 0;

    uint32_t tick = get_ticks();
    uint32_t f = irq_guard_save();

    uint32_t index;
    if (record_count < PROVENANCE_MAX_RECORDS)
    {
        index = record_count++;
    }
    else
    {
        index = (next_id - 1) % PROVENANCE_MAX_RECORDS;
        dropped++;
    }

    uint32_t id = next_id++;

    records[index].id = id;
    records[index].tick = tick;
    records[index].source = source;
    records[index].target = target;
    records[index].relation = relation;
    records[index].reason = reason;
    records[index].valid = 1;

    irq_guard_restore(f);
    return id;
}

int provenance_get(uint32_t id, provenance_record_t *out)
{
    int found = 0;

    if (!out || id == 0)
        return 0;

    uint32_t f = irq_guard_save();
    for (uint32_t i = 0; i < PROVENANCE_MAX_RECORDS; i++)
    {
        if (records[i].valid && records[i].id == id)
        {
            *out = records[i];
            found = 1;
            break;
        }
    }
    irq_guard_restore(f);

    return found;
}

void provenance_get_snapshot(provenance_snapshot_t *snapshot)
{
    if (!snapshot)
        return;

    uint32_t f = irq_guard_save();

    uint32_t start = oldest_slot();

    for (uint32_t k = 0; k < record_count; k++)
        snapshot->records[k] = records[(start + k) % PROVENANCE_MAX_RECORDS];

    snapshot->record_count = record_count;
    snapshot->dropped = dropped;

    irq_guard_restore(f);
}

uint32_t provenance_count(void)
{
    return record_count;
}

uint32_t provenance_dropped(void)
{
    return dropped;
}

uint32_t provenance_find_from(provenance_entity_ref_t source, provenance_record_t *results, uint32_t max_results)
{
    uint32_t found = 0;

    if (!results || max_results == 0)
        return 0;

    uint32_t f = irq_guard_save();

    uint32_t start = oldest_slot();

    for (uint32_t k = 0; k < record_count && found < max_results; k++)
    {
        const provenance_record_t *r = &records[(start + k) % PROVENANCE_MAX_RECORDS];
        if (r->valid && entity_equal(r->source, source))
            results[found++] = *r;
    }
    irq_guard_restore(f);

    return found;
}

uint32_t provenance_find_to(provenance_entity_ref_t target, provenance_record_t *results, uint32_t max_results)
{
    uint32_t found = 0;

    if (!results || max_results == 0)
        return 0;

    uint32_t f = irq_guard_save();

    uint32_t start = oldest_slot();

    for (uint32_t k = 0; k < record_count && found < max_results; k++)
    {
        const provenance_record_t *r = &records[(start + k) % PROVENANCE_MAX_RECORDS];
        if (r->valid && entity_equal(r->target, target))
            results[found++] = *r;
    }
    irq_guard_restore(f);

    return found;
}

uint32_t provenance_find_relation(provenance_entity_ref_t entity, provenance_relation_t relation, provenance_record_t *results, uint32_t max_results)
{
    uint32_t found = 0;

    if (!results || max_results == 0)
        return 0;

    uint32_t f = irq_guard_save();

    uint32_t start = oldest_slot();

    for (uint32_t k = 0; k < record_count && found < max_results; k++)
    {
        const provenance_record_t *r = &records[(start + k) % PROVENANCE_MAX_RECORDS];
        if (r->valid && r->relation == relation && (entity_equal(r->source, entity) || entity_equal(r->target, entity)))
            results[found++] = *r;
    }
    irq_guard_restore(f);

    return found;
}

static provenance_record_t trace_copy[PROVENANCE_MAX_RECORDS];
static provenance_entity_ref_t trace_queue[PROVENANCE_MAX_RECORDS + 1];
static provenance_entity_ref_t trace_visited[PROVENANCE_MAX_RECORDS + 1];

static uint32_t trace(provenance_entity_ref_t start, int forward, provenance_record_t *results, uint32_t max_results)
{
    uint32_t head = 0;
    uint32_t tail = 0;
    uint32_t visited_count = 0;
    uint32_t found = 0;

    if (!results || max_results == 0)
        return 0;

    uint32_t f = irq_guard_save();
    uint32_t start_slot = oldest_slot();
    uint32_t count = record_count;
    for (uint32_t k = 0; k < count; k++)
        trace_copy[k] = records[(start_slot + k) % PROVENANCE_MAX_RECORDS];
    irq_guard_restore(f);

    trace_queue[tail++] = start;
    trace_visited[visited_count++] = start;

    while (head < tail && found < max_results)
    {
        provenance_entity_ref_t current = trace_queue[head++];

        for (uint32_t k = 0; k < count && found < max_results; k++)
        {
            const provenance_record_t *r = &trace_copy[k];
            provenance_entity_ref_t from = forward ? r->source : r->target;

            provenance_entity_ref_t next = forward ? r->target : r->source;

            if (!r->valid || !entity_equal(from, current))
                continue;

            results[found++] = *r;

            int seen = 0;
            for (uint32_t j = 0; j < visited_count; j++)
            {
                if (entity_equal(trace_visited[j], next))
                {
                    seen = 1;
                    break;
                }
            }

            if (!seen && visited_count < PROVENANCE_MAX_RECORDS + 1)
            {
                trace_visited[visited_count++] = next;
                trace_queue[tail++] = next;
            }
        }
    }

    return found;
}

uint32_t provenance_trace_from(provenance_entity_ref_t start, provenance_record_t *results, uint32_t max_results)
{
    return trace(start, 1, results, max_results);
}

uint32_t provenance_trace_to(provenance_entity_ref_t target, provenance_record_t *results, uint32_t max_results)
{
    return trace(target, 0, results, max_results);
}