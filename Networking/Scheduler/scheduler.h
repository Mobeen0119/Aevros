#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include "../Bailiff/bailiff.h"

#define RETRY_LIMIT 8
#define SCHED_INITIAL_RTO 100
#define SCHED_MIN_RTO 20
#define SCHED_MAX_RTO 1000
#define SCHED_DUP_ACK_THRESHOLD 3
#define SCHED_MAX_FRAME BAILIFF_MAX_FRAME

typedef enum
{
    PACKET_STATE_IDLE = 0,
    PACKET_STATE_RETRY,
    PACKET_STATE_FAILURE,
    PACKET_STATE_DROPPED

} packet_sched_state_t;

typedef struct
{
    uint32_t conn_id;
    uint32_t seq;
    uint16_t length;

    uint32_t start_timer;
    uint32_t retry_track;
    packet_sched_state_t state;
    uint8_t frame[SCHED_MAX_FRAME];

    uint32_t srtt;
    uint32_t rttvar;
    uint32_t rto;
    int retransmitted;

    uint32_t last_ack_seen;
    uint32_t dup_ack_count;
} packets_state_t;

int scheduler_track(uint32_t conn_id, uint32_t seq, const uint8_t *frame, uint16_t len);
void scheduler_ack(uint32_t conn_id, uint32_t acked_seq);

void scheduler_cancel(uint32_t conn_id);
void scheduler_tick(void);

uint32_t scheduler_retransmit_count(void);
uint32_t scheduler_giveup_count(void);

#endif