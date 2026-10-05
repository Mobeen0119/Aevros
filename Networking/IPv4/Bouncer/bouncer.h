#ifndef BOUNCER_H
#define BOUNCER_H

#include <stdint.h>

typedef enum
{
    BOUNCE_ACCEPT = 0,
    BOUNCE_REJECT_TOO_SHORT, 

    BOUNCE_REJECT_TOO_LONG,    
    BOUNCE_REJECT_BAD_DEST,    
                            
} bounce_verdict_t;

#define BOUNCER_LOG_CAPACITY 64

typedef struct
{
    uint32_t tick;
    bounce_verdict_t verdict;
    uint16_t length;

    uint8_t dest_mac[6];
    uint8_t src_mac[6];
} bouncer_log_entry_t;

bounce_verdict_t bouncer_check(const uint8_t *frame, uint16_t length, const uint8_t our_mac[6]);

void bouncer_log(bounce_verdict_t verdict, uint16_t length, const uint8_t *dest_mac, const uint8_t *src_mac);

uint32_t bouncer_log_count(void);

const bouncer_log_entry_t *bouncer_log_get(uint32_t index);

uint32_t bouncer_turned_away_count(void);

const char *bounce_verdict_string(bounce_verdict_t v);

#endif