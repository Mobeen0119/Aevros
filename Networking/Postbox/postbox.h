#ifndef POSTBOX_H
#define POSTBOX_H

#include <stdint.h>

#define POSTBOX_MAX_PENDING 8
#define POSTBOX_MAX_DATAGRAM 512

int postbox_deposit(uint32_t slot, const uint8_t src_ip[4], uint16_t src_port,
                     const uint8_t *data, uint16_t len);

uint16_t postbox_read(uint32_t slot, uint8_t src_ip_out[4], uint16_t *src_port_out,
                       uint8_t *data_out, uint16_t max_len);

uint16_t postbox_pending_count(uint32_t slot);

#endif