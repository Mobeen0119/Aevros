#ifndef BAILIFF_H
#define BAILIFF_H

#include <stdint.h>

#define BAILIFF_MAX_PASSES 32
#define BAILIFF_PASS_TIMEOUT_TICKS 500
#define BAILIFF_MAX_FRAME 1600 

typedef struct
{
    uint32_t pass_id;
    uint16_t declared_len;
    uint32_t content_checksum;
    uint32_t issued_at;
    int in_use;
} hall_pass_t;

int bailiff_request_pass(const uint8_t *frame, uint16_t len, uint32_t *out_pass_id);

int bailiff_present_pass(uint32_t pass_id, const uint8_t *frame, uint16_t len);

uint32_t bailiff_denied_count(void);

uint32_t bailiff_transmitted_count(void); 

#endif