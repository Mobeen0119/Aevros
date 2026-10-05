#ifndef ATLAS6_H
#define ATLAS6_H

#include <stdint.h>

#define ATLAS6_MAX_ROUTES 16


typedef struct
{
    uint8_t network[16];
    uint8_t prefix_len;
    uint8_t gateway[16]; // all-zero means on-link, no next hop needed 
    int in_use;
} atlas6_route_t;

int atlas6_add_route(const uint8_t network[16], uint8_t prefix_len, const uint8_t gateway[16]);

int atlas6_remove_route(const uint8_t network[16], uint8_t prefix_len);

int atlas6_set_default_gateway(const uint8_t gateway[16]);

int atlas6_lookup(const uint8_t dest_ip[16], uint8_t out_next_hop[16]);

uint32_t atlas6_route_count(void);

#endif