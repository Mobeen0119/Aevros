#ifndef POSTCARD_H
#define POSTCARD_H

#include <stdint.h>

typedef enum{
    UDP_ACCEPT=0,
    UPD_REJECT_TOO_SHORT,
    UDP_REJECT_LENGTH_MISMATCH,
    UDP_REJECT_BAD_CHECKSUM
}udp_verdict_t;


void postcard_handle(const uint8_t *payload, uint16_t length, const uint8_t src_ip[4], const uint8_t dst_ip[4]);

uint32_t postcard_accepted_count(void);
uint32_t postcard_rejected_count(void);
const char *udp_verdict_string(udp_verdict_t v);



#endif
