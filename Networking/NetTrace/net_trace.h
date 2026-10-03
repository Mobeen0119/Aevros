#ifndef NETTRACE_H
#define NETTRACE_H

#include <stdint.h>
#include "../NetWatch/net_watch.h"

#define NETTRACE_MAX_EVENTS NETWATCH_MAX_EVENTS
#define NETTRACE_TIMEOUT_TICKS 500

typedef enum
{
    NETTRACE_STATUS_NONE,
    NETTRACE_STATUS_PENDING,
    NETTRACE_STATUS_ANSWERED,
    NETTRACE_STATUS_TIMED_OUT

} nettrace_status_t;
typedef enum
{
    NETTRACE_RELATION_NONE = 0,
    NETTRACE_RELATION_PREVIOUS,

    NETTRACE_RELATION_RELATED,
    NETTRACE_RELATION_RESPONSE

} nettrace_relation_t;

typedef struct
{
    uint32_t event_id, previous_id, related_id;

    netwatch_protocol_t protocol;
    netwatch_direction_t direction;

    uint32_t length;
    uint32_t src_ip, dst_ip;

    uint16_t src_port, dst_port;

    nettrace_relation_t relation;
    uint8_t valid;

    uint64_t tick;
    nettrace_status_t status;
} nettrace_event_t;

typedef struct
{
    nettrace_event_t events[NETTRACE_MAX_EVENTS];
    uint32_t event_count;

} nettrace_snapshot_t;

void nettrace_init(void);
void nettrace_rebuild(const netwatch_snapshot_t *snapshot);

const nettrace_event_t *nettrace_get_event(uint32_t index);
uint32_t nettrace_get_event_count(void);

void nettrace_get_snapshot(nettrace_snapshot_t *snapshot);

int32_t nettrace_find_event(uint32_t event_id);

int32_t nettrace_find_related(uint32_t event_id);
void nettrace_clear(void);

void nettrace_rebuild_at(const netwatch_snapshot_t *snapshot, uint64_t now);

#endif