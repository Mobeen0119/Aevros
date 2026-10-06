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

uint32_t provenance_record(uint32_t source, uint32_t target, provenance_relation_t relation, uint32_t reason)
{
    uint32_t index;

    if (record_count < PROVENANCE_MAX_RECORDS)
    {
        index = record_count++;
    }
    else
    {
        index = next_id % PROVENANCE_MAX_RECORDS;
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
