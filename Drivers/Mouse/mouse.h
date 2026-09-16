#ifndef PS2_MOUSE_H
#define PS2_MOUSE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int8_t  dx;          // signed X delta since last packet
    int8_t  dy;          // signed Y delta
    bool    left_button;
    bool    right_button;
    bool    middle_button;
} mouse_event_t;

typedef struct {
    uint64_t last_irq_tick;
    uint64_t packets_decoded;
    uint32_t queue_len;
} mouse_status_t;

void ps2_mouse_init(void);
void ps2_mouse_irq_handler(void);

bool ps2_mouse_poll_event(mouse_event_t *out);

bool ps2_mouse_selftest(void);

void ps2_mouse_status(mouse_status_t *out);

const char *ps2_mouse_dependency_note(void);

#endif