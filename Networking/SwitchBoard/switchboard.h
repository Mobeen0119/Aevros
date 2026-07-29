#ifndef SWITCHBOARD_H
#define SWITCHBOARD_H

#include <stdint.h>

#define SWITCHBOARD_NONE 0xFFFFFFFFu

int switchboard_bind(uint16_t port, uint8_t protocol);

uint32_t switchboard_accept(uint16_t port);

uint16_t switchboard_recv(uint32_t conn_id, uint8_t *out, uint16_t max_len);

uint16_t switchboard_recv_udp(uint16_t port, uint8_t src_ip_out[4], uint16_t *src_port_out,
                               uint8_t *data_out, uint16_t max_len);

#endif