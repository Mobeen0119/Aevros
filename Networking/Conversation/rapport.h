#ifndef RAPPORT_H
#define RAPPORT_H

#include <stdint.h>
#include "conversation.h"

void rapport_on_syn(uint32_t conn_id);

void rapport_on_ack(uint32_t conn_id);

void rapport_on_fin(uint32_t conn_id);

void rapport_on_rst(uint32_t conn_id);

uint32_t rapport_get_generation(uint32_t conn_id); 

conversation_state_t rapport_get_state(uint32_t conn_id);

const char *rapport_state_string(conversation_state_t s);

#endif