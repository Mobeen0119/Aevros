#ifndef ECHO_H
#define ECHO_H

#include <stdint.h>

#define ICMP_MIN_HEADER 8
#define ICMP_ECHO_REQUEST 8
#define ICMP_ECHO_REPLY 0
#define ECHO_MAX_PENDING 8
#define ECHO_MAX_PAYLOAD 512

typedef enum
{
    ICMP_ACCEPT,
    ICMP_REJECT_TOO_SHORT,
    ICMP_REJECT_BAD_CHECKSUM,
    ICMP_REJECT_NOT_ECHO_REQUEST,
    ICMP_REJECT_PAYLOAD_TOO_LARGE
} icmp_verdict_t;

void echo_handle(const uint8_t *payload, uint16_t length, const uint8_t src_ip[4], const uint8_t dst_ip[4]);

int echo_build_reply(uint8_t *out_buf, uint16_t *out_len, uint8_t reply_dst_ip[4]);

uint32_t echo_accepted_count(void);

uint32_t echo_rejected_count(void);

int echo_dispatch_reply(const uint8_t our_mac[6], const uint8_t our_ip[4], uint32_t *out_pass_id);

const char *icmp_verdict_string(icmp_verdict_t v);

const uint8_t *echo_last_frame(void);

uint16_t echo_last_len(void);

#endif