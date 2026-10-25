#ifndef WATCHLIST_H
#define WATCHLIST_H

#include <stdint.h>

#define WATCHLIST_CAPACITY 16
#define WATCH_WINDOW_TICKS 200 // 2 seconds at 100 Hz
#define WATCH_THRESHOLD 4000

int watchlist_observe(const uint8_t src_mac[6]);

int watchlist_is_flagged(const uint8_t src_mac[6]);

uint32_t watchlist_flagged_count(void);

uint32_t watchlist_dropped_count(void);

#endif
