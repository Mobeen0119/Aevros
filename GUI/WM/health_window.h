#ifndef HEALTH_WINDOW_H
#define HEALTH_WINDOW_H

#include <stdint.h>

void health_window_paint(int x, int y, int w, int h);

const char *health_window_status(uint32_t *color);

void health_window_why(char *buf, int cap);
typedef struct
{
    uint32_t free_frames;
    uint32_t live_allocs;
    uint32_t tasks;
    uint32_t seconds;
} health_sig_t;

health_sig_t health_window_signature(void);

#endif