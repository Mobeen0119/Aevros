#ifndef DIRECTORY_H
#define DIRECTORY_H

#include <stdint.h>

#define DIRECTORY_CLIENT_PORT 50053
#define DIRECTORY_SERVER_PORT 53
#define DIRECTORY_CACHE_CAPACITY 8
#define DIRECTORY_MAX_HOSTNAME 63
#define DIRECTORY_TIMEOUT_TICKS 300

void directory_start(void);

void directory_set_server(const uint8_t dns_ip[4]);

int directory_lookup_cached(const char *hostname, uint8_t out_ip[4]);

int directory_query(const char *hostname, const uint8_t our_mac[6], const uint8_t our_ip[4]);

void directory_tick(const uint8_t our_mac[6], const uint8_t our_ip[4]);

#endif