#ifndef COMPASS6_H
#define COMPASS6_H
#include <stdint.h>

typedef enum
{
    IP6_ACCEPT = 0,
    IP6_REJECT_TOO_SHORT,
    IP6_REJECT_BAD_VERSION,
    IP6_REJECT_LENGTH_MISMATCH,
    IP6_REJECT_FRAGMENTED

} ip6_verdict_t;

void compass6_handle(const uint8_t *payload, uint16_t length, const uint8_t src_mac[6]);

uint32_t compass6_accepted_count(void);

uint32_t compass6_rejected_count(void);

const char *ip6_verdict_string(ip6_verdict_t v);

#endif