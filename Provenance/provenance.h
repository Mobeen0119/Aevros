#ifndef PROVENANCE_H
#define PROVENANCE_H

#include <stdint.h>

#define PROVENANCE_MAX_RECORDS 256

typedef enum
{
    PROVENANCE_RELATION_NONE = 0,
    PROVENANCE_RELATION_CAUSED,
    PROVENANCE_RELATION_AFFECTED,
    PROVENANCE_RELATION_CREATED,
    PROVENANCE_RELATION_DESTROYED,

    PROVENANCE_RELATION_OWNED,
    PROVENANCE_RELATION_TRIGGERED,
    PROVENANCE_RELATION_DEPENDED_ON,
    PROVENANCE_RELATION_RESPONDED_TO

} provenance_relation_t;

typedef struct
{
    uint32_t id, source, target;
    provenance_relation_t relation;
    uint32_t reason;
    uint8_t valid;

} provenance_record_t;

typedef struct
{
    provenance_record_t records[PROVENANCE_MAX_RECORDS];
    uint32_t record_count;
} provenance_snapshot_t;

void provenance_init(void);

uint32_t provenance_record(uint32_t source, uint32_t target, provenance_relation_t relation, uint32_t reason);

const provenance_record_t *provenance_get(uint32_t id);

void provenance_get_snapshot(provenance_snapshot_t *snapshot);

uint32_t provenance_count(void);
void provenance_clear(void);

#endif