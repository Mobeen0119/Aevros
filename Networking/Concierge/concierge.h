#ifndef CONCIERGE_H
#define CONCIERGE_H

#include <stdint.h>

void concierge_tick(const uint8_t our_mac[6]);

void concierge_maybe_tick(const uint8_t our_mac[6]);

#endif