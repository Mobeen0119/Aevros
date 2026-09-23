#ifndef RENDER_H
#define RENDER_H

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    REDRAW_NEW,
    REDRAW_STATE_CHANGED,

    REDRAW_STACK_CHANGED,
    REDRAW_GEOMETRY_CHANGED,
    
    REDRAW_EXPOSED

} redraw_reason_t;

typedef struct
{
    uint32_t wid;
    redraw_reason_t reason;

    uint64_t tick;
} render_log_entry_t;

typedef enum
{
    REDRAW_NEW,
    REDRAW_STATE_CHANGED,
    REDRAW_STACK_CHANGED,
    REDRAW_GEOMETRY_CHANGED
} redraw_reason_t;

#define RENDER_LOG_LEN 16

void window_render(uint32_t wid);

void render_all_windows(void);

uint32_t render_get_log(render_log_entry_t *out, uint32_t max_entries);

const char *render_dependency_note(void);
bool render_selftest(void);

#endif