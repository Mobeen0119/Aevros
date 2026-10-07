#include "provenance.h"

static provenance_record_t records[PROVENANCE_MAX_RECORDS];

static uint32_t record_count;
static uint32_t next_id;

void provenance_init(void)
{
    record_count = 0;
    next_id = 1;

    for (uint32_t i = 0; i < PROVENANCE_MAX_RECORDS; i++)
        records[i].valid = 0;
}

uint32_t provenance_record(provenance_entity_ref_t source, provenance_entity_ref_t target, provenance_relation_t relation, provenance_reason_t reason)
{
    uint32_t index;

    if (record_count < PROVENANCE_MAX_RECORDS)
    {
        index = record_count++;
    }
    else
    {
        index = (next_id - 1) % PROVENANCE_MAX_RECORDS;
    }

    records[index].id = next_id++;
    records[index].source = source;
    records[index].target = target;

    records[index].relation = relation;
    records[index].reason = reason;

    records[index].valid = 1;

    return records[index].id;
}

const provenance_record_t *provenance_get(uint32_t id)
{
    for (uint32_t i = 0; i < PROVENANCE_MAX_RECORDS; i++)
    {
        if (records[i].valid && records[i].id == id)
            return &records[i];
    }

    return 0;
}

void provenance_get_snapshot(provenance_snapshot_t *snapshot)
{
    if (!snapshot)
        return;

    for (uint32_t i = 0; i < PROVENANCE_MAX_RECORDS; i++)
        snapshot->records[i] = records[i];

    snapshot->record_count = record_count;
}

uint32_t provenance_count(void)
{
    return record_count;
}

void provenance_clear(void)
{
    record_count = 0;
    next_id = 1;

    for (uint32_t i = 0; i < PROVENANCE_MAX_RECORDS; i++)
        records[i].valid = 0;
}

static int entity_equal(provenance_entity_ref_t a, provenance_entity_ref_t b)
{
    return a.type == b.type && a.id == b.id;
}

uint32_t provenance_find_from(provenance_entity_ref_t source, provenance_record_t *results, uint32_t max_results)
{
    uint32_t found = 0;

    if (!results || max_results == 0)
        return 0;

    for (uint32_t i = 0; i < PROVENANCE_MAX_RECORDS && found < max_results; i++)
    {
        if (records[i].valid && entity_equal(records[i].source, source))
            results[found++] = records[i];
    }

    return found;
}

uint32_t provenance_find_to(provenance_entity_ref_t target, provenance_record_t *results, uint32_t max_results)
{
    uint32_t found = 0;

    if (!results || max_results == 0)
        return 0;

    for (uint32_t i = 0; i < PROVENANCE_MAX_RECORDS && found < max_results; i++)
    {
        if (records[i].valid && entity_equal(records[i].target, target))
            results[found++] = records[i];
    }

    return found;
}

uint32_t provenance_find_relation(provenance_entity_ref_t entity, provenance_relation_t relation, provenance_record_t *results, uint32_t max_results)
{
    uint32_t found = 0;

    if (!results || max_results == 0)
        return 0;

    for (uint32_t i = 0; i < PROVENANCE_MAX_RECORDS && found < max_results; i++)
    {
        if (records[i].valid && records[i].relation == relation && (entity_equal(records[i].source, entity) || entity_equal(records[i].target, entity)))
        {
            results[found++] = records[i];
        }
    }

    return found;
}
