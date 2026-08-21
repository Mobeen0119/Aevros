#ifndef POSTBOX6_H
#define POSTBOX6_H

#include <stdint.h>

#define POSTBOX6_MAX_PENDING 8
#define POSTBOX6_MAX_DATAGRAM 512

int postbox6_deposit(uint32_t slot, const uint8_t src_ip[16], uint16_t src_port,
                      const uint8_t *data, uint16_t len);

uint16_t postbox6_read(uint32_t slot, uint8_t src_ip_out[16], uint16_t *src_port_out,
                        uint8_t *data_out, uint16_t max_len);

uint16_t postbox6_pending_count(uint32_t slot);

#endif