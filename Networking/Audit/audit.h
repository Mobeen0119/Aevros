#ifndef AUDIT_H
#define AUDIT_H
#include <stdint.h>

#define AUDIT_LOG_CAPACITY 64

typedef struct
{
    uint32_t tick;
    uint8_t event_type;
    uint8_t ip[4];

} audit_record_t;

void audit_start(void);

uint32_t audit_count(void);

int audit_get(uint32_t i, audit_record_t *out);

void audit_dump(void);
#endif
