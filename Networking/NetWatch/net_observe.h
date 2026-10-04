#ifndef NET_OBSERVE_H
#define NET_OBSERVE_H

#include <stdint.h>
#include <stdbool.h>
#include "net_watch.h"

typedef struct
{
    uint32_t captured;  // frames handed to capture()
    uint32_t dropped;   // queue full, frame not observed
    uint32_t recorded;  // frames became a netwatch event
    uint32_t malformed; // frames rejected by parser
    uint32_t unknown;   // well-formed Ethernet frames with an ethertype we don't track
    const char *last_malformed_reason;
} netobserve_stats_t;

void netobserve_init(void);

void netobserve_capture(netwatch_direction_t direction, const void *frame, uint32_t length);

uint32_t netobserve_drain(void);

void netobserve_get_stats(netobserve_stats_t *out);

#endif