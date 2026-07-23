#ifndef CONVERSATION_H
#define CONVERSATION_H


#include <stdint.h>

typedef enum{
    TCP_ACCEPT=0,
    TCP_REJECT_TOO_SHORT,
    TCP_REJECT_BAD_HEADER_LENGTH,
    TCP_REJECT_LENGTH_MISMATCH,
    TCP_REJECT_BAD_CHECKSUM,
    TCP_REJECT_NONSENSE_FLAGS
}tcp_verdict_t;


typedef enum
{
    CONV_CLOSED = 0,
    CONV_LISTEN,
    CONV_SYN_SENT,
    CONV_SYN_RECEIVED,
    CONV_ESTABLISHED,
    CONV_FIN_WAIT,
    CONV_CLOSING,
    CONV_TIME_WAIT,
} conversation_state_t;

void conversation_handle(const uint8_t *payload, uint16_t length, const uint8_t src_ip[4], const uint8_t dst_ip[4]);

uint32_t tcp_accepted_count(void);

uint32_t tcp_rejected_count(void);

const char* tcp_verdict_string(tcp_verdict_t v);

#endif