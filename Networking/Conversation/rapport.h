#ifndef RAPPORT_H
#define RAPPORT_H

#include <stdint.h>
#include "conversation.h"


void rapport_on_syn(uint32_t conn_id, uint32_t isn);

void rapport_on_ack(uint32_t conn_id);

void rapport_on_fin(uint32_t conn_id);

void rapport_on_rst(uint32_t conn_id);

int rapport_seq_expected(uint32_t conn_id, uint32_t seq);

void rapport_advance_seq(uint32_t conn_id, uint16_t amount);

conversation_state_t rapport_get_state(uint32_t conn_id);

int rapport_seq_is_stale_retransmit(uint32_t conn_id, uint32_t seq, uint16_t data_len);

const char *rapport_state_string(conversation_state_t s);

#endif