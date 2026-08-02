#ifndef POSTBOX_H
#define POSTBOX_H

#include <stdint.h>

#define POSTBOX_MAX_PENDING 8
#define POSTBOX_MAX_DATAGRAM 512
#define POSTCARD_MAX_PAYLOAD 1472



int postbox_deposit(uint32_t slot, const uint8_t src_ip[4], uint16_t src_port,
                     const uint8_t *data, uint16_t len);

uint16_t postbox_read(uint32_t slot, uint8_t src_ip_out[4], uint16_t *src_port_out,
                       uint8_t *data_out, uint16_t max_len);

int postcard_dispatch(const uint8_t dest_ip[4], uint16_t dest_port, uint16_t src_port,const uint8_t our_mac[6], const uint8_t our_ip[4],
                     const uint8_t *data, uint16_t len);

uint16_t postbox_pending_count(uint32_t slot);

#endif
