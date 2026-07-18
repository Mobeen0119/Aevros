#ifndef WATCHLIST_H
#define WATCHLIST_H

#include <stdint.h>

#define WATCHLIST_CAPACITY   16
#define WATCH_WINDOW_TICKS   200  
#define WATCH_THRESHOLD      50    


int watchlist_observe(const uint8_t src_mac[6]);

int watchlist_is_flagged(const uint8_t src_mac[6]);

uint32_t watchlist_flagged_count(void);

#endif
