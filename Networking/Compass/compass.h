#ifndef COMPASS_H
#define COMPASS_H

#include <stdint.h>

typedef enum
{
    IP_ACCEPT = 0,
    IP_REJECT_TOO_SHORT,
    IP_REJECT_BAD_VERSION,
    IP_REJECT_BAD_HEADER_LEN,
    IP_REJECT_LENGTH_MISMATCH,
    IP_REJECT_FRAGMENTED,
    IP_REJECT_BAD_CHECKSUM,
} ip_verdict_t;

void compass_handle(const uint8_t* payload,uint16_t length);

uint32_t compass_rejected_count(void);
uint32_t compass_accepted_count(void);

const char* ip_verdict_string(ip_verdict_t v);

#endif