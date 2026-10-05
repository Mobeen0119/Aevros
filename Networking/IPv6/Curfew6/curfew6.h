#ifndef CURFEW6_H
#define CURFEW6_H

#include <stdint.h>

#define CURFEW6_CAPACITY 64
#define CURFEW6_WINDOW_TICKS 200
#define CURFEW6_THRESHOLD 150

int curfew6_check(const uint8_t src_ip[16]);

uint32_t curfew6_denied_count(void);

#endif