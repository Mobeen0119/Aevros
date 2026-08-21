#ifndef LANDLORD6_H
#define LANDLORD6_H

#include <stdint.h>

#define LANDLORD6_CLIENT_PORT 546
#define LANDLORD6_SERVER_PORT 547


typedef enum
{
    LANDLORD6_IDLE = 0,
    LANDLORD6_INFO_REQUESTING,
    LANDLORD6_BOUND 
} landlord6_state_t;

#define LANDLORD6_MAX_DNS_SERVERS 2

void landlord6_start(const uint8_t our_mac[6], const uint8_t our_ip[16]);

void landlord6_tick(const uint8_t our_mac[6], const uint8_t our_ip[16]);

landlord6_state_t landlord6_get_state(void);

uint32_t landlord6_get_dns_servers(uint8_t out_ips[][16]);

#endif