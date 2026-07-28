#include "rapport.h"
#include "../LockBox/lockbox.h"
#include "../../Lib/kprintf.h"

static conversation_state_t status[LOCKBOX_CAPACITY];

static uint32_t expected_seq[LOCKBOX_CAPACITY];

static uint32_t our_isn_table[LOCKBOX_CAPACITY];

static int valid_id(uint32_t conn_id)
{
    return conn_id < LOCKBOX_CAPACITY;
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

void rapport_on_ack(uint32_t conn_id)
{
    if (!valid_id(conn_id))
        return;

    if (status[conn_id] == CONV_SYN_RECEIVED)
    {
        status[conn_id] = CONV_ESTABLISHED;
        kprintf("[Rapport] slot %d: we're officially talking now\n", conn_id);
    }
}

void rapport_on_fin(uint32_t conn_id)
{
    if (!valid_id(conn_id))
        return;

    if (status[conn_id] == CONV_CLOSED)
    {
        kprintf("[Rapport] slot %d: already said goodbye once, ignoring the echo\n", conn_id);

        return;
    }

    kprintf("[Rapport] slot %d: said their goodbyes, letting go of the slot\n", conn_id);

    status[conn_id] = CONV_CLOSING;
    lockbox_release(conn_id);
    status[conn_id] = CONV_CLOSED;
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
    lockbox_release(conn_id);
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
    case CONV_CLOSING:
        return "wrapping up";
    case CONV_TIME_WAIT:
        return "wrapping up";
    default:
        return "unknown (this shouldn't happen)";
    }
}
