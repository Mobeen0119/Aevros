#ifndef CONVERSATION6_H
#define CONVERSATION6_H

#include <stdint.h>
#include "../Conversation/conversation.h"

void conversation6_handle(const uint8_t *payload, uint16_t length, const uint8_t src_ip[16], const uint8_t dst_ip[16]);

uint32_t tcp6_accepted_count(void);

uint32_t tcp6_rejected_count(void);

int conversation6_dispatch_syn_ack(uint32_t conn_id, const uint8_t our_mac[6], const uint8_t our_ip[16], uint32_t *out_pass_id);

int conversation6_dispatch_ack(uint32_t conn_id, const uint8_t our_mac[6], const uint8_t our_ip[16], uint32_t *out_pass_id);

int conversation6_dispatch_syn(uint32_t conn_id, uint32_t our_isn, const uint8_t our_mac[6], const uint8_t our_ip[16], uint32_t *out_pass_id);

int conversation6_dispatch_data(uint32_t conn_id, const uint8_t *data, uint16_t len, const uint8_t our_mac[6], const uint8_t our_ip[16], uint32_t *out_pass_id);

int conversation6_dispatch_fin(uint32_t conn_id, const uint8_t our_mac[6], const uint8_t our_ip[16], uint32_t *out_pass_id);

const uint8_t *conversation6_last_frame(void);

uint16_t conversation6_last_len(void);

#endif