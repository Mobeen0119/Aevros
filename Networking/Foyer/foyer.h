#ifndef FOYER_H
#define FOYER_H

#include <stdint.h>
#include "../Bailiff/bailiff.h"

#define FOYER_CAPACITY 16
#define FOYER_MAX_FRAME BAILIFF_MAX_FRAME
#define FOYER_MAX_RETRIES 5
#define FOYER_RETRY_INTERVAL_TICK 100

int foyer_queue(const uint8_t target_ip[4], const uint8_t out_mac[6], const uint8_t *frame, uint16_t len);

void foyer_tick(const uint8_t our_mac[6]);

uint32_t foyer_flushed_count(void);

uint32_t foyer_dropped_count(void);

#endif
