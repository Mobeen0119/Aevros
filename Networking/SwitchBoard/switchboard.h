#ifndef SWITCHBOARD_H
#define SWITCHBOARD_H

#include <stdint.h>

#define SWITCHBOARD_NONE 0xFFFFFFFFu

int switchboard_bind(uint16_t port, uint8_t protocol);

uint32_t switchboard_accept(uint16_t port);

uint16_t switchboard_recv(uint32_t conn_id, uint8_t *out, uint16_t max_len);

int switchboard_connect(const uint8_t dest_ip[4], uint16_t dest_port, const uint8_t our_mac[6], const uint8_t our_ip[4], uint32_t *out_conn_id);

int switchboard_send(uint32_t conn_id, const uint8_t *data, uint16_t len, const uint8_t our_mac[6], const uint8_t our_ip[4]);

int switchboard_close(uint32_t conn_id, const uint8_t our_mac[6], const uint8_t our_ip[4]);

uint16_t switchboard_recv_udp(uint16_t port, uint8_t src_ip_out[4], uint16_t *src_port_out,
                              uint8_t *data_out, uint16_t max_len);

#endif