#ifndef LANDLORD_H
#define LANDLORD_H

#include <stdint.h>

#define LANDLORD_CLIENT_PORT 68
#define LANDLORD_SERVER_PORT 67

typedef enum
{
    LANDLORD_INIT = 0,
    LANDLORD_DISCOVERING,
    LANDLORD_REQUESTING,
    LANDLORD_BOUND
} landlord_state_t;

void landlord_start(const uint8_t our_mac[6]);

void landlord_tick(const uint8_t our_mac[6]);

landlord_state_t landlord_get_state(void);

#endif