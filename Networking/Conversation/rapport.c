#include "rapport.h"
#include "../LockBox/lockbox.h"
#include "../../Lib/kprintf.h"

static conversation_state_t status[LOCKBOX_CAPACITY];

static int valid_id(uint32_t conn_id)
{
    return conn_id < LOCKBOX_CAPACITY;
}

void rapport_on_syn(uint32_t conn_id)
{
    if (!valid_id(conn_id))
        return;

    status[conn_id] = CONV_SYN_RECEIVED;
    kprintf("[Rapport] slot %d: said hi, waiting to hear back\n", conn_id);
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