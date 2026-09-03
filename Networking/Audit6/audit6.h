#ifndef AUDIT6_H
#define AUDIT6_H
#include <stdint.h>

#define AUDIT6_LOG_CAPACITY 64

typedef struct
{
    uint32_t tick;
    uint8_t event_type;
    uint8_t ip[16];
} audit6_record_t;

void audit6_start(void);

uint32_t audit6_count(void);

int audit6_get(uint32_t i, audit6_record_t *out);

void audit6_dump(void);
#endif