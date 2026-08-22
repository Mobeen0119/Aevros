#include "frontdoor.h"
#include "../SwitchBoard/switchboard.h"
#include "../Conversation/rapport.h"
#include "../LockBox/lockbox.h"
#include "../FrontDesk/frontdesk.h"
#include "../Rolodex/rolodex.h"
#include "../../Lib/string.h"
#include "../../Lib/kprintf.h"

typedef enum
{
    SOCK_FREE = 0,
    SOCK_TCP,
    SOCK_UDP

} sock_kind_t;

typedef struct
{
    sock_kind_t kind;
    int listening;
    uint32_t conn_id;
    uint16_t local_port;
    int in_use;

} socket_entry_t;

static socket_entry_t sockets[FRONTDOOR_MAX_SOCKETS];

#define UDP_EPHEMERAL_LOW 40000
#define UDP_EPHEMERAL_HIGH 40999
static uint16_t next_udp_ephemeral = UDP_EPHEMERAL_LOW;

static void get_identity(uint8_t our_mac[6], uint8_t our_ip[4])
{

    memcpy(our_mac, frontdesk_get_state()->mac, 6);
    rolodex_get_ip(our_ip);
}

static int find_free_slot(void)
{

    for (int i = 0; i < FRONTDOOR_MAX_SOCKETS; i++)
        if (!sockets[i].in_use)
            return i;

    return -1;
}

static uint16_t pick_udp_ephemeral(void)
{
    for (uint32_t tries = 0; tries < (UDP_EPHEMERAL_HIGH - UDP_EPHEMERAL_LOW); tries++)
    {
        uint16_t candidate = next_udp_ephemeral;
        next_udp_ephemeral = (next_udp_ephemeral >= UDP_EPHEMERAL_HIGH) ? UDP_EPHEMERAL_LOW : (uint16_t)(next_udp_ephemeral + 1);

        if (lockbox_find_listener(candidate, 17) == LOCKBOX_CAPACITY)
            return candidate;
    }
    return 0;
}

void frontdoor_init(void)
{
    for (int i = 0; i < FRONTDOOR_MAX_SOCKETS; i++)
        sockets[i].in_use = 0;
}

void frontdoor_tick(void)
{
}

int frontdoor_socket(int type)
{

    if (type != SOCK_TYPE_TCP && type != SOCK_TYPE_UDP)
        return -1;

    int slot = find_free_slot();
    if (slot < 0)
    {
        kprintf("[Frontdoor] socket table full\n");
    }

    sockets[slot].kind = (type == SOCK_TYPE_TCP) ? SOCK_TCP : SOCK_UDP;
    sockets[slot].listening = 0;

    sockets[slot].conn_id = LOCKBOX_CAPACITY;

    sockets[slot].local_port = 0;

    sockets[slot].in_use = 1;

    return slot;
}

int frontdoor_bind(int fd, uint16_t port)
{

    if (fd < 0 || fd >= FRONTDOOR_MAX_SOCKETS || !sockets[fd].in_use)
        return -1;

    uint8_t protocol = (sockets[fd].kind == SOCK_TCP) ? 6 : 17;
    if (!switchboard_bind(port, protocol))
        return -1;

    sockets[fd].local_port = port;

    if (sockets[fd].kind == SOCK_TCP)
        sockets[fd].listening = 1;

    return 0;
}

int frontdoor_connect(int fd, const sock_addr_t *addr)
{

    if (fd < 0 || fd >= FRONTDOOR_MAX_SOCKETS || !sockets[fd].in_use || sockets[fd].kind != SOCK_TCP)
        return -1;

    uint8_t our_mac[6], our_ip[4];

    get_identity(our_mac, our_ip);

    uint32_t conn_id;

    if (!switchboard_connect(addr->ip, addr->port, our_mac, our_ip, &conn_id))
        return -1;

    sockets[fd].conn_id = conn_id;

    return 0;
}

int frontdoor_accept(int fd)
{
    if (fd < 0 || fd >= FRONTDOOR_MAX_SOCKETS || !sockets[fd].in_use || !sockets[fd].listening)
        return -1;

    uint32_t conn_id = switchboard_accept(sockets[fd].local_port);
    if (conn_id == SWITCHBOARD_NONE)
        return -1;

    int slot = find_free_slot();

    if (slot < 0)
        return -1;

    sockets[slot].kind = SOCK_TCP;

    sockets[slot].listening = 0;
    sockets[slot].conn_id = conn_id;

    sockets[slot].local_port = sockets[fd].local_port;

    sockets[slot].in_use = 1;

    return slot;
}

int frontdoor_send(int fd, const uint8_t *buf, uint16_t len)
{
    if (fd < 0 || fd >= FRONTDOOR_MAX_SOCKETS || !sockets[fd].in_use || sockets[fd].kind != SOCK_TCP || sockets[fd].listening)
        return -1;

    uint8_t our_mac[6], our_ip[4];

    get_identity(our_mac, our_ip);

    if (!switchboard_send(sockets[fd].conn_id, buf, len, our_mac, our_ip))
        return -1;

    return len;
}

int frontdoor_recv(int fd, uint8_t *buf, uint16_t max_len)
{

    if (fd < 0 || fd >= FRONTDOOR_MAX_SOCKETS || !sockets[fd].in_use || sockets[fd].kind != SOCK_TCP || sockets[fd].listening)
        return -1;

    uint16_t got = switchboard_recv(sockets[fd].conn_id, buf, max_len);

    if (got > 0)
        return got;

    conversation_state_t st = rapport_get_state(sockets[fd].conn_id);

    if (st == CONV_CLOSED || st == CONV_TIME_WAIT)
        return -1;

    return 0;
}

int frontdoor_sendto(int fd, const sendto_args_t *args)
{
    if (fd < 0 || fd >= FRONTDOOR_MAX_SOCKETS || !sockets[fd].in_use || sockets[fd].kind != SOCK_UDP)
        return -1;

    if (sockets[fd].local_port == 0)
    {
        uint16_t port = pick_udp_ephemeral();

        if (port == 0 || !switchboard_bind(port, 17))
            return -1;

        sockets[fd].local_port = port;
    }
    uint8_t our_mac[6], our_ip[4];

    get_identity(our_mac, our_ip);

    if (!switchboard_send_udp(sockets[fd].local_port, args->dest.ip, args->dest.port, our_mac, our_ip, args->buf, args->len))
        return -1;

    return args->len;
}

int frontdoor_recvfrom(int fd, recvfrom_args_t *args)
{

    if (fd < 0 || fd >= FRONTDOOR_MAX_SOCKETS || !sockets[fd].in_use || sockets[fd].kind != SOCK_UDP)
        return -1;

    return switchboard_recv_udp(sockets[fd].local_port, args->src_out.ip, &args->src_out.port, args->buf, args->max_len);
}

int frontdoor_close(int fd)
{
    if (fd < 0 || fd >= FRONTDOOR_MAX_SOCKETS || !sockets[fd].in_use)
        return -1;

    if (sockets[fd].kind == SOCK_TCP && !sockets[fd].listening && sockets[fd].conn_id != LOCKBOX_CAPACITY)
    {

        conversation_state_t st = rapport_get_state(sockets[fd].conn_id);

        if (st == CONV_ESTABLISHED || st == CONV_CLOSE_WAIT)
        {
            uint8_t our_mac[6], our_ip[4];

            get_identity(our_mac, our_ip);

            switchboard_close(sockets[fd].conn_id, our_mac, our_ip);
        }
    }

    sockets[fd].in_use = 0;

    return 0;
}
