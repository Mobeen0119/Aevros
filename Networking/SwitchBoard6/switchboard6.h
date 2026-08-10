#ifndef SWITCHBOARD6_H
#define SWITCHBOARD6_H

#include <stdint.h>
#define SWITCHBOARD6_NONE 0xFFFFFFFFu

int switchboard6_bind(uint16_t port);

uint32_t switchboard6_accept(uint16_t port);

uint16_t switchboard6_recv(uint32_t conn_id, uint8_t *out, uint16_t max_len);

int switchboard6_connect(const uint8_t dest_ip[16], uint16_t dest_port, const uint8_t our_mac[6], const uint8_t our_ip[16], uint32_t *out_conn_id);

int switchboard6_send(uint32_t conn_id, const uint8_t *data, uint16_t len, const uint8_t our_mac[6], const uint8_t our_ip[16]);

int switchboard6_close(uint32_t conn_id, const uint8_t our_mac[6], const uint8_t our_ip[16]);

int switchboard6_send_udp(uint16_t local_port, const uint8_t dest_ip[16], uint16_t dest_port,const uint8_t our_mac[6], const uint8_t our_ip[16],const uint8_t *data, uint16_t len);

#endif