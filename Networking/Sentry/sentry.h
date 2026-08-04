#ifndef SENTRY_H
#define SENTRY_H

#include <stdint.h>

#define SENTRY_CAPACITY 16

#define SENTRY_PORT_HISTORY 8

#define SENTRY_WINDOW_TICKS 200

#define SENTRY_DISTINCT_PORT_THRESHOLD 6

#define SENTRY_BAN_TICKS 3000

int sentry_observe(const uint8_t src_ip[4], uint16_t dst_port);

uint32_t sentry_flagged_count(void);

#endif