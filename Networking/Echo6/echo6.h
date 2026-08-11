#ifndef ECHO6_H
#define ECHO6_H

#include <stdint.h>

#define ICMP6_MIN_HEADER 8
#define ECHO6_MAX_PAYLOAD 64
#define ECHO6_MAX_PENDING 8

#define ICMP6_ECHO_REQUEST 128
#define ICMP6_ECHO_REPLY 129
#define ICMP6_NEIGHBOR_SOLICITATION 135
#define ICMP6_NEIGHBOR_ADVERTISEMENT 136

typedef enum
{
    ICMP6_ACCEPT = 0,
    ICMP6_REJECT_TOO_SHORT,
    ICMP6_REJECT_BAD_CHECKSUM,
    ICMP6_REJECT_UNHANDLED_TYPE,
    ICMP6_REJECT_PAYLOAD_TOO_LARGE,
    
} icmp6_verdict_t;

void echo6_handle(const uint8_t *payload, uint16_t length, const uint8_t src_ip[16], const uint8_t dst_ip[16]);

int echo6_dispatch_reply(const uint8_t our_mac[6], const uint8_t our_ip[16], uint32_t *out_pass_id);

const uint8_t *echo6_last_frame(void);

int echo6_dispatch_neighbor_solicitation(const uint8_t target_ip[16], const uint8_t our_mac[6], const uint8_t our_ip[16], uint32_t *out_pass_id);

uint16_t echo6_last_len(void);

uint32_t echo6_accepted_count(void);

uint32_t echo6_rejected_count(void);

const char *icmp6_verdict_string(icmp6_verdict_t v);

#endif