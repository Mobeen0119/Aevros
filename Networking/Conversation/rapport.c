#include "rapport.h"
#include "../LockBox/lockbox.h"
#include "../Scheduler/scheduler.h"
#include "../../kernel/Process/task.h"
#include "../../Lib/kprintf.h"

static conversation_state_t status[LOCKBOX_CAPACITY];

static uint32_t expected_seq[LOCKBOX_CAPACITY];

static uint32_t our_isn_table[LOCKBOX_CAPACITY];

static uint32_t our_fin_seq[LOCKBOX_CAPACITY];
static uint32_t time_wait_started[LOCKBOX_CAPACITY];

static uint16_t peer_window[LOCKBOX_CAPACITY];

static int valid_id(uint32_t conn_id)
{
    return conn_id < LOCKBOX_CAPACITY;
}

static void enter_time_wait(uint32_t conn_id)
{
    status[conn_id] = CONV_TIME_WAIT;
    time_wait_started[conn_id] = get_ticks();
    kprintf("[Rapport] slot %d: both sides done, waiting out TIME_WAIT before the slot is free\n", conn_id);
}

void rapport_on_syn(uint32_t conn_id, uint32_t peer_isn, uint32_t our_isn)
{
    if (!valid_id(conn_id))
        return;

    status[conn_id] = CONV_SYN_RECEIVED;
    expected_seq[conn_id] = peer_isn + 1;

    our_isn_table[conn_id] = our_isn;

    kprintf("[Rapport] slot %d: said hi, waiting to hear back (peer_isn=%u, our_isn=%u)\n", conn_id, peer_isn, our_isn);
}

void rapport_initiate_close(uint32_t conn_id, uint32_t fin_seq)
{
    if (!valid_id(conn_id))
        return;

    our_fin_seq[conn_id] = fin_seq;

    if (status[conn_id] == CONV_ESTABLISHED)
    {
        status[conn_id] = CONV_FIN_WAIT_1;
        kprintf("[Rapport] slot %d: closing our end, said our goodbyes, waiting for a reply\n", conn_id);
    }
    else if (status[conn_id] == CONV_CLOSE_WAIT)
    {
        status[conn_id] = CONV_LAST_ACK;
        kprintf("[Rapport] slot %d: peer already said goodbye, now we're saying ours\n", conn_id);
    }
}

void rapport_on_ack(uint32_t conn_id, uint32_t ack_num)
{
    if (!valid_id(conn_id))
        return;

    switch (status[conn_id])
    {
    case CONV_SYN_RECEIVED:
        status[conn_id] = CONV_ESTABLISHED;
        kprintf("[Rapport] slot %d: we're officially talking now\n", conn_id);
        break;

    case CONV_FIN_WAIT_1:
        if (ack_num >= our_fin_seq[conn_id] + 1)
        {
            status[conn_id] = CONV_FIN_WAIT_2;
            kprintf("[Rapport] slot %d: our goodbye was heard, waiting for theirs\n", conn_id);
        }
        break;

    case CONV_CLOSING:
        if (ack_num >= our_fin_seq[conn_id] + 1)
            enter_time_wait(conn_id);
        break;

    case CONV_LAST_ACK:
        if (ack_num >= our_fin_seq[conn_id] + 1)
        {
            kprintf("[Rapport] slot %d: our goodbye was heard, letting go of the slot\n", conn_id);
            status[conn_id] = CONV_CLOSED;
            scheduler_cancel(conn_id);
            lockbox_release(conn_id);
        }
        break;

    default:
        break;
    }
}

void rapport_on_fin(uint32_t conn_id)
{
    if (!valid_id(conn_id))
        return;

    switch (status[conn_id])
    {
    case CONV_ESTABLISHED:
        expected_seq[conn_id]++;
        status[conn_id] = CONV_CLOSE_WAIT;
        kprintf("[Rapport] slot %d: peer said their goodbyes, we're still allowed to talk until we say ours\n", conn_id);
        break;

    case CONV_FIN_WAIT_1:
        expected_seq[conn_id]++;
        status[conn_id] = CONV_CLOSING;
        kprintf("[Rapport] slot %d: both sides said goodbye at once\n", conn_id);
        break;

    case CONV_FIN_WAIT_2:
        expected_seq[conn_id]++;
        enter_time_wait(conn_id);
        break;

    case CONV_CLOSED:
        kprintf("[Rapport] slot %d: already said goodbye once, ignoring the echo\n", conn_id);
        break;

    default:
        kprintf("[Rapport] slot %d: goodbye received again mid-close, ignoring the echo\n", conn_id);
        break;
    }
}

void rapport_on_rst(uint32_t conn_id)
{
    if (!valid_id(conn_id))
        return;

    if (status[conn_id] == CONV_CLOSED)
    {
        kprintf("[Rapport] slot %d: already hung up on once, ignoring the echo\n", conn_id);
        return;
    }

    kprintf("[Rapport] slot %d: got hung up on, letting go of the slot\n", conn_id);
    status[conn_id] = CONV_CLOSED;
    scheduler_cancel(conn_id);
    lockbox_release(conn_id);
}

void rapport_tick(void)
{
    uint32_t now = get_ticks();

    for (uint32_t i = 0; i < LOCKBOX_CAPACITY; i++)
    {
        if (status[i] != CONV_TIME_WAIT)
            continue;

        if (now - time_wait_started[i] < CONV_TIME_WAIT_TICKS)
            continue;

        kprintf("[Rapport] slot %u: TIME_WAIT expired, letting go of the slot\n", i);
        status[i] = CONV_CLOSED;
        lockbox_release(i);
    }
}

void rapport_set_peer_window(uint32_t conn_id, uint16_t window)
{
    if (!valid_id(conn_id))
        return;

    peer_window[conn_id] = window;
}

uint16_t rapport_get_peer_window(uint32_t conn_id)
{
    if (!valid_id(conn_id))
        return;

    return peer_window[conn_id];
}

void rapport_initiate_connect(uint32_t conn_id, uint32_t our_isn)
{
    if (!valid_id(conn_id))
        return;

    status[conn_id] = CONV_SYN_SENT;
    our_isn_table[conn_id] = our_isn;

    kprintf("[Rapport] slot %d: reaching out, said hi, waiting for a reply (our_isn=%u)\n", conn_id, our_isn);
}

int rapport_on_syn_ack(uint32_t conn_id, uint32_t peer_isn, uint32_t ack_num)
{
    if (!valid_id(conn_id))
        return 0;

    if (status[conn_id] != CONV_SYN_SENT)
        return 0;

    if (ack_num != our_isn_table[conn_id] + 1)
    {
        kprintf("[Rapport] slot %d: syn-ack didn't acknowledge our syn, ignoring\n", conn_id);
        return 0;
    }

    expected_seq[conn_id] = peer_isn + 1;
    status[conn_id] = CONV_ESTABLISHED;

    kprintf("[Rapport] slot %d: they said hi back, we're officially talking now\n", conn_id);

    return 1;
}

int rapport_send_allowed(uint32_t conn_id, uint16_t length)
{
    if (!valid_id(conn_id))
        return 0;

    uint32_t in_flight = scheduler_bytes_in_flight(conn_id);
    uint32_t window = peer_window[conn_id];

    return (in_flight + length) <= window;
}

int rapport_seq_expected(uint32_t conn_id, uint32_t seq)
{
    if (!valid_id(conn_id))
        return 0;

    return seq == expected_seq[conn_id];
}

uint32_t rapport_get_expected_seq(uint32_t conn_id)
{
    if (!valid_id(conn_id))
        return 0;

    return expected_seq[conn_id];
}

uint32_t rapport_get_our_isn(uint32_t conn_id)
{
    if (!valid_id(conn_id))
        return 0;

    return our_isn_table[conn_id];
}

int rapport_seq_is_stale_retransmit(uint32_t conn_id, uint32_t seq, uint16_t data_len)
{
    if (!valid_id(conn_id))
        return 0;

    return (seq < expected_seq[conn_id]) && (seq + data_len <= expected_seq[conn_id]);
}

void rapport_advance_seq(uint32_t conn_id, uint16_t amount)
{
    if (!valid_id(conn_id))
        return;

    expected_seq[conn_id] += amount;
}

conversation_state_t rapport_get_state(uint32_t conn_id)
{
    if (!valid_id(conn_id))
        return CONV_CLOSED;

    return status[conn_id];
}

const char *rapport_state_string(conversation_state_t s)
{
    switch (s)
    {
    case CONV_CLOSED:
        return "closed";
    case CONV_LISTEN:
        return "listening";
    case CONV_SYN_SENT:
        return "syn sent";
    case CONV_SYN_RECEIVED:
        return "said hi, waiting to hear back";
    case CONV_ESTABLISHED:
        return "mid-conversation";
    case CONV_FIN_WAIT:
        return "wrapping up";
    case CONV_FIN_WAIT_1:
        return "said our goodbyes, waiting for a reply";
    case CONV_FIN_WAIT_2:
        return "goodbye heard, waiting for theirs";
    case CONV_CLOSE_WAIT:
        return "they said goodbye, we haven't yet";
    case CONV_LAST_ACK:
        return "said our goodbyes second, waiting for a reply";
    case CONV_CLOSING:
        return "both said goodbye at once";
    case CONV_TIME_WAIT:
        return "wrapping up";
    default:
        return "unknown (this shouldn't happen)";
    }
}
