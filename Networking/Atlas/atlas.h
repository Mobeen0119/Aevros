#ifndef ATLAS_H
#define ATLAS_H

#include <stdint.h>

#define ATLAS_MAX_ROUTES 16

typedef struct
{
    uint8_t gateway[4];
    uint8_t netmask[4];
    uint8_t network[4];
    int in_use;
} atlas_route_t;

int atlas_add_route(const uint8_t network[4], const uint8_t netmask[4], const uint8_t gateway[4]);

int atlas_remove_route(const uint8_t network[4], const uint8_t netmask[4]);

int atlas_set_default_gateway(const uint8_t gateway[4]);

int atlas_lookup(const uint8_t dest_ip[4], uint8_t out_next_hop[4]);

#endif