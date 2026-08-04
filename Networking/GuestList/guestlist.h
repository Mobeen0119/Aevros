#ifndef GUESLITST_H
#define GUESLITST_H

#include <stdint.h>

#define GUESTLIST_CAPACITY 32


typedef enum
{
    GUESTLIST_NO_RULE = 0, 
    GUESTLIST_ALLOWED,
    GUESTLIST_DENIED
} guestlist_verdict_t;


int guestlist_set(const uint8_t ip[4],guestlist_verdict_t v);

void guestlist_clear(const uint8_t ip[4]);

guestlist_verdict_t guestlist_check(const uint8_t ip[4]);

uint32_t guestlist_count(void);

int guestlist_set_times(const uint8_t ip[4],guestlist_verdict_t v,uint32_t duration_ticks);

void guestlist_tick(void);

#endif