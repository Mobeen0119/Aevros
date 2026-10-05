#ifndef SENTRY6_H
#define SENTRY6_H

#include <stdint.h>

#define SENTRY6_CAPACITY 16
#define SENTRY6_PORT_HISTORY 8
#define SENTRY6_WINDOW_TICKS 200
#define SENTRY6_DISTINCT_PORT_THRESHOLD 6
#define SENTRY6_BAN_TICKS 3000

int sentry6_observe(const uint8_t src_ip[16], uint16_t dst_port);

uint32_t sentry6_flagged_count(void);

#endif