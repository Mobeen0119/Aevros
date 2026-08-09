#ifndef SCHEDULER6_H
#define SCHEDULER6_H

#include <stdint.h>
#include "../Bailiff/bailiff.h"

#define SCHED6_RETRY_LIMIT 8
#define SCHED6_INITIAL_RTO 100
#define SCHED6_MIN_RTO 20
#define SCHED6_MAX_RTO 1000
#define SCHED6_DUP_ACK_THRESHOLD 3
#define SCHED6_MAX_FRAME BAILIFF_MAX_FRAME

typedef enum
{
    PACKET6_STATE_IDLE = 0,
    PACKET6_STATE_RETRY,
    PACKET6_STATE_FAILURE,
    PACKET6_STATE_DROPPED

} packet6_sched_state_t;

typedef struct
{
    uint32_t conn_id;
    uint32_t seq;
    uint16_t length;

    uint32_t start_timer;
    uint32_t retry_track;
    packet6_sched_state_t state;
    uint8_t frame[SCHED6_MAX_FRAME];

    uint32_t srtt;
    uint32_t rttvar;
    uint32_t rto;
    int retransmitted;

    uint32_t last_ack_seen;
    uint32_t dup_ack_count;
} packets6_state_t;

int scheduler6_track(uint32_t conn_id, uint32_t seq, const uint8_t *frame, uint16_t len);
void scheduler6_ack(uint32_t conn_id, uint32_t acked_seq);

void scheduler6_cancel(uint32_t conn_id);
void scheduler6_tick(void);

uint32_t scheduler6_retransmit_count(void);
uint32_t scheduler6_giveup_count(void);

uint16_t scheduler6_bytes_in_flight(uint32_t conn_id);

#endif