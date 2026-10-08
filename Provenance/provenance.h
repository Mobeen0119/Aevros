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

typedef enum
{
    PROVENANCE_REASON_NONE = 0,
    PROVENANCE_REASON_INTERRUPT,
    PROVENANCE_REASON_SCHEDULE,

    PROVENANCE_REASON_ALLOCATION,
    PROVENANCE_REASON_DEALLOCATION,

    PROVENANCE_REASON_CREATION,
    PROVENANCE_REASON_DESTRUCTION,
    PROVENANCE_REASON_REQUEST,

    PROVENANCE_REASON_RESPONSE,
    PROVENANCE_REASON_DELIVERY,

    PROVENANCE_REASON_FAILURE,
    PROVENANCE_REASON_DEPENDENCY,

    PROVENANCE_REASON_USER_ACTION,
    PROVENANCE_REASON_SYSTEM_ACTION
} provenance_reason_t;

typedef enum
{
    PROVENANCE_ENTITY_NONE = 0,
    PROVENANCE_ENTITY_INTERRUPT,
    PROVENANCE_ENTITY_DRIVER,

    PROVENANCE_ENTITY_PROCESS,
    PROVENANCE_ENTITY_MEMORY,

    PROVENANCE_ENTITY_NETWORK,
    PROVENANCE_ENTITY_FILE,
    PROVENANCE_ENTITY_DEVICE,

    PROVENANCE_ENTITY_OPERATION
} provenance_entity_t;

typedef struct
{
    provenance_entity_t type;
    uint32_t id;
} provenance_entity_ref_t;

typedef struct
{
    uint32_t id;
    uint32_t tick;
    provenance_entity_ref_t source;
    provenance_entity_ref_t target;

    provenance_relation_t relation;
    provenance_reason_t reason;
    uint8_t valid;

} provenance_record_t;

typedef struct
{
    provenance_record_t records[PROVENANCE_MAX_RECORDS];
    uint32_t record_count;
    uint32_t dropped;

} provenance_snapshot_t;

void provenance_init(void);

uint32_t provenance_record(provenance_entity_ref_t source, provenance_entity_ref_t target, provenance_relation_t relation, provenance_reason_t reason);

int provenance_get(uint32_t id, provenance_record_t *out);

void provenance_get_snapshot(provenance_snapshot_t *snapshot);

uint32_t provenance_count(void);
uint32_t provenance_dropped(void);
void provenance_clear(void);

uint32_t provenance_find_from(provenance_entity_ref_t source, provenance_record_t *results, uint32_t max_results);

uint32_t provenance_find_to(provenance_entity_ref_t target, provenance_record_t *results, uint32_t max_results);

uint32_t provenance_find_relation(provenance_entity_ref_t entity, provenance_relation_t relation, provenance_record_t *results, uint32_t max_results);

#endif