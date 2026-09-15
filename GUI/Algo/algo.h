#ifndef ALGO_H
#define ALGO_H
#include <stdint.h>

void fb_line(int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b);
void fb_rect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b);

void fb_rect_filled(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b);
void fb_circle(int cx, int cy, int radius, uint8_t r, uint8_t g, uint8_t b);
void fb_circle_filled(int cx, int cy, int radius, uint8_t r, uint8_t g, uint8_t b);


#endif