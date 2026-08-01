#include "scheduler.h"
#include "../LockBox/lockbox.h"
#include "../Conversation/rapport.h"
#include "../../kernel/Process/task.h"
#include "../../Lib/string.h"
#include "../../Lib/kprintf.h"

static packets_state_t entries[LOCKBOX_CAPACITY];
static uint32_t retransmit_total;
static uint32_t giveup_total;

static int valid_id(uint32_t conn_id)
{
    return conn_id < LOCKBOX_CAPACITY;
}

static int resend(packets_state_t *e, uint32_t conn_id, const char *reason)
{
    uint32_t pass_id;

    if (bailiff_request_pass(e->frame, e->length, &pass_id) &&
        bailiff_present_pass(pass_id, e->frame, e->length))
    {
        retransmit_total++;
        kprintf("[Scheduler] slot %u: %s, resending seq %u\n", conn_id, reason, e->seq);
        return 1;
    }

    kprintf("[Scheduler] slot %u: retransmit blocked by Bailiff, will retry\n", conn_id);
    return 0;
}

int scheduler_track(uint32_t conn_id, uint32_t seq, const uint8_t *frame, uint16_t len)
{
    if (!valid_id(conn_id) || len == 0 || len > SCHED_MAX_FRAME)
        return 0;

    packets_state_t *e = &entries[conn_id];

    e->conn_id = conn_id;
    e->seq = seq;
    e->length = len;
    memcpy(e->frame, frame, len);
    e->start_timer = get_ticks();
    e->retry_track = 0;
    e->state = PACKET_STATE_RETRY;
    e->retransmitted = 0;
    e->dup_ack_count = 0;

    if (e->rto == 0)
        e->rto = SCHED_INITIAL_RTO;

    kprintf("[Scheduler] slot %u: tracking seq %u, %u bytes, waiting for ack\n", conn_id, seq, len);

    return 1;
}

void scheduler_ack(uint32_t conn_id, uint32_t acked_seq)
{
    if (!valid_id(conn_id))
        return;

    packets_state_t *e = &entries[conn_id];

    if (e->state != PACKET_STATE_RETRY)
        return;

    if (acked_seq >= e->seq + e->length)
    {
        if (!e->retransmitted)
        {
            uint32_t sample = get_ticks() - e->start_timer;

            if (e->srtt == 0)
            {
                e->srtt = sample;
                e->rttvar = sample / 2;
            }
            else
            {
                uint32_t diff = (sample > e->srtt) ? (sample - e->srtt) : (e->srtt - sample);
                e->rttvar = e->rttvar - (e->rttvar / 4) + (diff / 4);
                e->srtt = e->srtt - (e->srtt / 8) + (sample / 8);
            }

            e->rto = e->srtt + 4 * e->rttvar;
            if (e->rto < SCHED_MIN_RTO)
                e->rto = SCHED_MIN_RTO;
            if (e->rto > SCHED_MAX_RTO)
                e->rto = SCHED_MAX_RTO;

            kprintf("[Scheduler] slot %u: rtt sample %u ticks, rto now %u\n", conn_id, sample, e->rto);
        }

        kprintf("[Scheduler] slot %u: ack for seq %u arrived, clearing retransmit timer\n", conn_id, acked_seq);
        e->state = PACKET_STATE_IDLE;
        e->retry_track = 0;
        e->dup_ack_count = 0;
        e->last_ack_seen = acked_seq;
        return;
    }

    if (acked_seq == e->last_ack_seen)
    {
        e->dup_ack_count++;

        kprintf("[Scheduler] slot %u: duplicate ack for %u (%u in a row)\n", conn_id, acked_seq, e->dup_ack_count);

        if (e->dup_ack_count >= SCHED_DUP_ACK_THRESHOLD)
        {
            resend(e, conn_id, "3 duplicate acks, fast retransmit");
            e->dup_ack_count = 0;
        }
    }
    else
    {
        e->last_ack_seen = acked_seq;
        e->dup_ack_count = 0;
    }
}

void scheduler_cancel(uint32_t conn_id)
{
    if (!valid_id(conn_id))
        return;

    packets_state_t *e = &entries[conn_id];

    e->state = PACKET_STATE_IDLE;
    e->retry_track = 0;
    e->rto = 0;
    e->srtt = 0;
    e->rttvar = 0;
    e->dup_ack_count = 0;
}

void scheduler_tick(void)
{
    uint32_t now = get_ticks();

    for (uint32_t i = 0; i < LOCKBOX_CAPACITY; i++)
    {
        packets_state_t *e = &entries[i];

        if (e->state != PACKET_STATE_RETRY)
            continue;

        if (now - e->start_timer < e->rto)
            continue;

        if (e->retry_track >= RETRY_LIMIT)
        {
            kprintf("[Scheduler] slot %u: gave up after %u retries, seq %u never acked\n",
                    i, e->retry_track, e->seq);

            e->state = PACKET_STATE_FAILURE;
            giveup_total++;

            rapport_on_rst(i);
            continue;
        }

        resend(e, i, "timed out");

        e->retransmitted = 1;
        e->rto = (e->rto * 2 > SCHED_MAX_RTO) ? SCHED_MAX_RTO : e->rto * 2;
        e->retry_track++;
        e->start_timer = now;
    }
}

uint16_t scheduler_bytes_in_flight(uint32_t conn_id){
    if(!valid_id(conn_id)) return 0;

    return (entries[conn_id].state == PACKET_STATE_RETRY) ? entries[conn_id].length : 0;
}
uint32_t scheduler_retransmit_count(void)
{
    return retransmit_total;
}

uint32_t scheduler_giveup_count(void)
{
    return giveup_total;
}