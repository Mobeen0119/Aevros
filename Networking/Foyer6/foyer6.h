#ifndef FOYER6_H
#define FOYER6_H
#include <stdint.h>
#include "../Bailiff/bailiff.h"

#define FOYER6_CAPACITY 16
#define FOYER6_MAX_FRAME BAILIFF_MAX_FRAME
#define FOYER6_MAX_RETRIES 5
#define FOYER6_RETRY_INTERVAL_TICK 100

int foyer6_queue(const uint8_t target_ip[16], const uint8_t our_mac[6], const uint8_t our_ip[16], const uint8_t *frame, uint16_t len);

void foyer6_tick(const uint8_t our_mac[6], const uint8_t our_ip[16]);
uint32_t foyer6_flushed_count(void);

uint32_t foyer6_dropped_count(void);

#endif