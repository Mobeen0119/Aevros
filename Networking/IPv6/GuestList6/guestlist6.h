#ifndef GUESTLIST6_H
#define GUESTLIST6_H

#include <stdint.h>
#define GUESTLIST6_CAPACITY 32

typedef enum
{
    GUESTLIST6_NO_RULE = 0,
    GUESTLIST6_ALLOWED,
    GUESTLIST6_DENIED
} guestlist6_verdict_t;

int guestlist6_set(const uint8_t ip[16], guestlist6_verdict_t v);

void guestlist6_clear(const uint8_t ip[16]);

guestlist6_verdict_t guestlist6_check(const uint8_t ip[16]);

uint32_t guestlist6_count(void);

int guestlist6_set_timed(const uint8_t ip[16], guestlist6_verdict_t v, uint32_t duration_ticks);

void guestlist6_tick(void);

#endif