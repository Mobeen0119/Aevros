#ifndef WHY_H
#define WHY_H

#include <stdint.h>

typedef enum
{
    WHY_NONE = 0,
    WHY_SEND_WAITING_ACK,   // Pending ACK
    WHY_CLOSE_WAITING_ACK,  // Close delayed by unacked data
    WHY_OUT_OF_ORDER,       // Packet arrived early
    WHY_STALE_RETRANSMIT,   // Duplicate packet received
    WHY_RETRANSMIT_TIMEOUT, // RTO expired ... resending
    WHY_PEER_WINDOW_FULL,   // Receiver window full
    WHY_COUNT
} why_reason_t;

void why_init(void);

int why_note(uint32_t conn_id, why_reason_t reason);

uint32_t why_count(uint32_t conn_id, why_reason_t reason);

why_reason_t why_top_reason(uint32_t conn_id);

const char *why_reason_text(why_reason_t reason);

void why_report(void);

#endif