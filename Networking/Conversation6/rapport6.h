#ifndef RAPPORT6_H
#define RAPPORT6_H

#include <stdint.h>
#include "conversation6.h"

#define CONV6_TIME_WAIT_TICKS 400

void rapport6_on_syn(uint32_t conn_id, uint32_t peer_isn, uint32_t our_isn);

void rapport6_on_ack(uint32_t conn_id, uint32_t ack_num);

void rapport6_initiate_close(uint32_t conn_id, uint32_t fin_seq);

void rapport6_on_fin(uint32_t conn_id);

void rapport6_on_rst(uint32_t conn_id);

void rapport6_tick(void);

int rapport6_seq_expected(uint32_t conn_id, uint32_t seq);

uint32_t rapport6_get_expected_seq(uint32_t conn_id);

uint32_t rapport6_get_our_isn(uint32_t conn_id);

int rapport6_seq_is_stale_retransmit(uint32_t conn_id, uint32_t seq, uint16_t data_len);

void rapport6_advance_seq(uint32_t conn_id, uint16_t amount);

uint32_t rapport6_get_send_seq(uint32_t conn_id);

void rapport6_advance_send_seq(uint32_t conn_id, uint16_t amount);

conversation_state_t rapport6_get_state(uint32_t conn_id);

const char *rapport6_state_string(conversation_state_t s);

void rapport6_set_peer_window(uint32_t conn_id, uint16_t window);

void rapport6_initiate_connect(uint32_t conn_id, uint32_t our_isn);

int rapport6_on_syn_ack(uint32_t conn_id, uint32_t peer_isn, uint32_t ack_num);

uint16_t rapport6_get_peer_window(uint32_t conn_id);

int rapport6_send_allowed(uint32_t conn_id, uint16_t length);

#endif