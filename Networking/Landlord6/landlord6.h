#ifndef LANDLORD6_H
#define LANDLORD6_H

#include <stdint.h>

#define LANDLORD6_CLIENT_PORT 68
#define LANDLORD6_SERVER_PORT 67

typedef enum
{
    LANDLORD6_INIT = 0,
    LANDLORD6_DISCOVERING,
    LANDLORD6_REQUESTING,
    LANDLORD6_BOUND
} landlord6_state_t;

void landlord6_start(const uint8_t our_mac[6]);

void landlord6_tick(const uint8_t our_mac[6]);

landlord6_state_t landlord6_get_state(void);

void landlord6_get_dns_server(uint8_t out_ip[6]);

#endif