#ifndef WAYSTATION_H
#define WAYSTATION_H

#include <stdint.h>

#define WAYSTATION_MAX_SEGMENT 1460
#define WAYSTATION_MAX_PENDING 4

int waystation_hold(uint32_t conn_id, uint32_t seq, const uint8_t *payload, uint16_t len);

int waystation_drain(uint32_t conn_id);

uint16_t waystation_receive_window(uint32_t conn_id);

uint16_t waystation_pending_count(uint32_t conn_id);

#endif