#ifndef CURFEW_H
#define CURFEW_H

#include <stdint.h>

#define CURFEW_CAPACITY 64
#define CURFEW_WINDOW_TICKS 200
#define CURFEW_THRESHOLD 150

int curfew_check(const uint8_t src_ip[4]);

uint32_t curfew_denied_count(void);

#endif