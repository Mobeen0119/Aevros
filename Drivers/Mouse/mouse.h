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

void ps2_mouse_init(void);

void ps2_mouse_irq_handler(void);


bool ps2_mouse_poll_event(mouse_event_t *out);


bool ps2_mouse_selftest(void);

#endif