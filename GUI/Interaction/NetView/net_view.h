#ifndef NETWORK_VIEW_H
#define NETWORK_VIEW_H

#include <stdint.h>
#include "../../Core/Panel/panel.h"

#define NETWORK_VIEW_X 0
#define NETWORK_VIEW_Y 0
#define NETWORK_VIEW_WIDTH 1024
#define NETWORK_VIEW_HEIGHT 768

#define NETWORK_VIEW_MAX_EVENTS 32

typedef enum
{
    NETWORK_VIEW_PROTO_NONE = 0,
    NETWORK_VIEW_PROTO_ARP,

    NETWORK_VIEW_PROTO_IPV4,
    NETWORK_VIEW_PROTO_IPV6,
    NETWORK_VIEW_PROTO_ICMP,
    NETWORK_VIEW_PROTO_UDP,
    NETWORK_VIEW_PROTO_TCP
} network_view_protocol_t;

typedef enum
{
    NETWORK_VIEW_RX = 0,
    NETWORK_VIEW_TX
} network_view_direction_t;

typedef struct
{
    uint32_t id;

    network_view_protocol_t protocol;
    network_view_direction_t direction;

    uint32_t length;

    uint32_t src_ip, dst_ip;

    uint16_t src_port, dst_port;

} network_view_event_t;

typedef struct
{
    uint8_t visible;
    uint8_t link_up;

    uint32_t rx_packets;
    uint32_t tx_packets;

    uint32_t rx_bytes;
    uint32_t tx_bytes;

    uint32_t arp_packets;
    uint32_t ipv4_packets;

    uint32_t ipv6_packets;

    uint32_t icmp_packets;
    uint32_t udp_packets;
    uint32_t tcp_packets;

    network_view_event_t events[NETWORK_VIEW_MAX_EVENTS];

    uint32_t event_count;

} network_view_state_t;

void network_view_init(void);

void network_view_show(void);

void network_view_hide(void);

void network_view_toggle(void);

uint8_t network_view_is_visible(void);

void network_view_update(const netwatch_snapshot_t *state);

void network_view_draw(void);

void network_view_mouse(int32_t x, int32_t y, uint8_t buttons);

void network_view_key(uint8_t key);

void network_view_select_event(uint32_t index);

int32_t network_view_selected_event(void);
void network_view_clear_selection(void);

void network_view_draw_event_detail(void);

void network_view_reset(void);

panel_rect_t netview_panel_rect(void);
void netview_handle_click(int32_t x, int32_t y);

#endif