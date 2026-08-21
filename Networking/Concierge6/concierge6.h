#ifndef CONCIERGE6_H
#define CONCIERGE6_H

#include <stdint.h>

typedef enum
{
    CONCIERGE6_SOLICITING_ROUTER = 0,
    CONCIERGE6_READY
} concierge6_state_t;

void concierge6_start(const uint8_t our_mac[6]);

void concierge6_tick(const uint8_t our_mac[6]);

concierge6_state_t concierge6_get_state(void);
void concierge6_get_global_ip(uint8_t out_ip[16]);

#endif