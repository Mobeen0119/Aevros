#ifndef WAYSTATION6_H
#define WAYSTATION6_H

#include <stdint.h>

#define WAYSTATION6_MAX_SEGMENT 1440
#define WAYSTATION6_MAX_PENDING 4

int waystation6_hold(uint32_t conn_id, uint32_t seq, const uint8_t *payload, uint16_t len);

int waystation6_drain(uint32_t conn_id);

uint16_t waystation6_receive_window(uint32_t conn_id);

uint16_t waystation6_pending_count(uint32_t conn_id);

#endif
