#ifndef MOUSE_CURSOR_BRIDGE_H
#define MOUSE_CURSOR_BRIDGE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint64_t events_forwarded;
    uint64_t clicks_resolved;
    uint64_t last_pump_tick;
    
} bridge_status_t;

void bridge_init(int32_t start_x, int32_t start_y);

void bridge_pump(void);
void bridge_status(bridge_status_t *out);

const char *bridge_dependency_note(void);

#endif