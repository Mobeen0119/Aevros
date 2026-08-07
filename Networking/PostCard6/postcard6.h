#ifndef POSTCARD6_H
#define POSTCARD6_H

#include <stdint.h>

#define UDP6_HEADER_LEN 8
#define POSTCARD6_MAX_PAYLOAD 1432

typedef enum
{
    UDP6_ACCEPT = 0,
    UDP6_REJECT_TOO_SHORT,
    UDP6_REJECT_LENGTH_MISMATCH,
    UDP6_REJECT_BAD_CHECKSUM,

} udp6_verdict_t;

void postcard6_handle(const uint8_t *payload, uint16_t length, const uint8_t src_ip[16], const uint8_t dst_ip[16]);

int postcard6_dispatch(const uint8_t dest_ip[16], uint16_t dest_port, uint16_t src_port, const uint8_t our_mac[6],
                       const uint8_t our_ip[16], const uint8_t *data, uint16_t len);

uint32_t postcard6_accepted_count(void);

uint32_t postcard6_rejected_count(void);

const char *udp6_verdict_string(udp6_verdict_t v);

#endif