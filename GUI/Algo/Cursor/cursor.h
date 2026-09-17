#ifndef CURSOR_H

#define CURSOR_H

#include <stdint.h>
#include <stdbool.h>
#include "mouse.h"

typedef enum {
    CURSOR_IDLE,
    CURSOR_MOVING,

    CURSOR_ACTIVE
} cursor_visual_state_t;

typedef struct {
    int32_t x;
    int32_t y;

    bool    visible;

} cursor_state_t;

typedef struct {
    uint64_t last_update_tick;
    uint64_t moves_applied;
    int32_t  x;

    int32_t  y;
    cursor_visual_state_t visual_state;


} cursor_status_t;


typedef struct {
    int32_t x;

    int32_t y;

    cursor_visual_state_t visual_state;

    uint64_t tick;

} cursor_history_entry_t;

void cursor_init(int32_t start_x, int32_t start_y);

void cursor_update(mouse_event_t ev);

void cursor_hide(void);

void cursor_show(void);

void cursor_status(cursor_status_t *out);

const char *cursor_dependency_note(void);

uint32_t cursor_get_history(cursor_history_entry_t *out, uint32_t max_entries);

bool cursor_selftest(void);



#endif