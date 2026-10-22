#include "scheduler.h"
#include "../IPv4/LockBox/lockbox.h"
#include "../IPv4/Conversation/rapport.h"
#include "../../kernel/Process/task.h"
#include "../../Lib/string.h"
#include "../../Lib/kprintf.h"
#include "../Why/why.h"

static packets_state_t conns[LOCKBOX_CAPACITY];
static uint32_t retransmit_total;
static uint32_t giveup_total;

static int valid_id(uint32_t conn_id)
{
    return conn_id < LOCKBOX_CAPACITY;
}

static int seq_before(uint32_t a, uint32_t b)
{
    return (int32_t)(a - b) < 0;
}

static int resend(sched_segment_t *e, uint32_t conn_id, const char *reason)
{
    uint32_t pass_id;

    if (bailiff_request_pass(e->frame, e->length, &pass_id) &&
        bailiff_present_pass(pass_id, e->frame, e->length))
    {
        retransmit_total++;
        why_note(conn_id, WHY_RETRANSMIT_TIMEOUT);
        kprintf("[Scheduler] slot %u: %s, resending seq %u\n", conn_id, reason, e->seq);
        return 1;
    }

    kprintf("[Scheduler] slot %u: retransmit blocked by Bailiff, will retry\n", conn_id);
    return 0;
}

static sched_segment_t *oldest(packets_state_t *c)
{
    sched_segment_t *best = 0;

    for (int i = 0; i < SCHED_WINDOW; i++)
        if (c->seg[i].in_use && (!best || seq_before(c->seg[i].seq, best->seq)))
            best = &c->seg[i];

    return best;
}

static void clear_window(packets_state_t *c)
{
    for (int i = 0; i < SCHED_WINDOW; i++)
        c->seg[i].in_use = 0;

    c->state = PACKET_STATE_IDLE;
    c->retry_track = 0;
    c->dup_ack_count = 0;
}

int scheduler_track(uint32_t conn_id, uint32_t seq, uint16_t seq_len, const uint8_t *frame, uint16_t len)
{
    if (!valid_id(conn_id) || len == 0 || len > SCHED_MAX_FRAME)
        return 0;

    packets_state_t *c = &conns[conn_id];
    sched_segment_t *e = 0;

    for (int i = 0; i < SCHED_WINDOW; i++)
        if (!c->seg[i].in_use)
        {
            e = &c->seg[i];
            break;
        }

    if (!e)
        return 0;

    e->seq = seq;
    e->length = len;
    e->seq_len = seq_len;
    memcpy(e->frame, frame, len);
    e->start_timer = get_ticks();
    e->retransmitted = 0;
    e->in_use = 1;

    if (c->state != PACKET_STATE_RETRY)
    {
        c->state = PACKET_STATE_RETRY;
        c->retry_track = 0;
        c->dup_ack_count = 0;
    }

    if (c->rto == 0)
        c->rto = SCHED_INITIAL_RTO;

    return 1;
}

void scheduler_ack(uint32_t conn_id, uint32_t acked_seq)
{
    if (!valid_id(conn_id))
        return;

    packets_state_t *c = &conns[conn_id];

    if (c->state != PACKET_STATE_RETRY)
        return;

    int freed = 0;
    int have_sample = 0;
    uint32_t sample = 0;

    for (int i = 0; i < SCHED_WINDOW; i++)
    {
        sched_segment_t *e = &c->seg[i];

        if (!e->in_use || seq_before(acked_seq, e->seq + e->seq_len))
            continue;

        if (!e->retransmitted && !have_sample)
        {
            sample = get_ticks() - e->start_timer;
            have_sample = 1;
        }

        e->in_use = 0;
        freed++;
    }

    if (freed)
    {
        if (have_sample)
        {
            if (c->srtt == 0)
            {
                c->srtt = sample;
                c->rttvar = sample / 2;
            }
            else
            {
                uint32_t diff = (sample > c->srtt) ? (sample - c->srtt) : (c->srtt - sample);
                c->rttvar = c->rttvar - (c->rttvar / 4) + (diff / 4);
                c->srtt = c->srtt - (c->srtt / 8) + (sample / 8);
            }

            c->rto = c->srtt + 4 * c->rttvar;
            if (c->rto < SCHED_MIN_RTO)
                c->rto = SCHED_MIN_RTO;
            if (c->rto > SCHED_MAX_RTO)
                c->rto = SCHED_MAX_RTO;
        }

        c->retry_track = 0;
        c->dup_ack_count = 0;
        c->last_ack_seen = acked_seq;

        if (!oldest(c))
            c->state = PACKET_STATE_IDLE;

        return;
    }

    if (acked_seq == c->last_ack_seen)
    {
        c->dup_ack_count++;

        if (c->dup_ack_count >= SCHED_DUP_ACK_THRESHOLD)
        {
            sched_segment_t *first = oldest(c);

            if (first)
                resend(first, conn_id, "3 duplicate acks, fast retransmit");

            c->dup_ack_count = 0;
        }
    }
    else
    {
        c->last_ack_seen = acked_seq;
        c->dup_ack_count = 0;
    }
}

void scheduler_cancel(uint32_t conn_id)
{
    if (!valid_id(conn_id))
        return;

    packets_state_t *c = &conns[conn_id];

    clear_window(c);
    c->rto = 0;
    c->srtt = 0;
    c->rttvar = 0;
}

void scheduler_tick(void)
{
    uint32_t now = get_ticks();

    for (uint32_t i = 0; i < LOCKBOX_CAPACITY; i++)
    {
        packets_state_t *c = &conns[i];

        if (c->state != PACKET_STATE_RETRY)
            continue;

        sched_segment_t *e = oldest(c);

        if (!e)
        {
            c->state = PACKET_STATE_IDLE;
            continue;
        }

        if (now - e->start_timer < c->rto)
            continue;

        if (c->retry_track >= RETRY_LIMIT)
        {
            kprintf("[Scheduler] slot %u: gave up after %u retries, seq %u never acked\n", i, c->retry_track, e->seq);

            clear_window(c);
            c->state = PACKET_STATE_FAILURE;
            giveup_total++;

            rapport_on_rst(i);
            continue;
        }

        resend(e, i, "timed out");

        for (int k = 0; k < SCHED_WINDOW; k++)
            c->seg[k].retransmitted = 1;

        c->rto = (c->rto * 2 > SCHED_MAX_RTO) ? SCHED_MAX_RTO : c->rto * 2;
        c->retry_track++;
        e->start_timer = now;
    }
}

uint16_t scheduler_bytes_in_flight(uint32_t conn_id)
{
    if (!valid_id(conn_id))
        return 0;

    uint32_t total = 0;

    for (int i = 0; i < SCHED_WINDOW; i++)
        if (conns[conn_id].seg[i].in_use)
            total += conns[conn_id].seg[i].seq_len;

    return total > 0xFFFF ? 0xFFFF : (uint16_t)total;
}

uint8_t scheduler_free_slots(uint32_t conn_id)
{
    if (!valid_id(conn_id))
        return 0;

    uint8_t free_slots = 0;

    for (int i = 0; i < SCHED_WINDOW; i++)
        if (!conns[conn_id].seg[i].in_use)
            free_slots++;

    return free_slots;
}

uint32_t scheduler_retransmit_count(void)
{
    return retransmit_total;
}

uint32_t scheduler_giveup_count(void)
{
    return giveup_total;
}