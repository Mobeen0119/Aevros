#ifndef HEALTH_WINDOW_H
#define HEALTH_WINDOW_H

#include <stdint.h>

void health_window_paint(int x, int y, int w, int h);

const char *health_window_status(uint32_t *color);

void health_window_why(char *buf, int cap);

#endif