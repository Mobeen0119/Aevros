#ifndef BOUNCER_H

#define BOUNCER_H

#include <stdint.h>

typedef enum
{
    BOUNCE_ACCEPT = 0,
    BOUNCE_REJECT_TOO_SHORT,
    BOUNCE_REJECT_TOO_LONG,
    BOUNCE_REJECT_BAD_DEST
} bouncer_verdict_t;

#define BOUNCER_LOG_CAPACITY 64

typedef struct
{
    uint32_t tick;
    bouncer_verdict_t verdict;
    uint16_t length;
    uint8_t dest_mac, src_mac;

} bouncer_log_entry_t;

bouncer_verdict_t  bouncer_check(const uint8_t frame,uint16_t length,const uint8_t our_mac[6]);

void bouncer_log(bouncer_verdict_t v,uint16_t lenght, const uint8_t *dest_mac,const uint8_t *src_mac);

uint32_t bouncer_log_count(void);

const bouncer_log_entry_t *bouncer_log_get(uint32_t index);

uint32_t bouncer_turned_away_count(void);

const char* bouncer_verdict_string(bouncer_verdict_t v);

#endif