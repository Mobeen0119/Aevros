#include "algo.h"

#include "../Framebuffer/framebuffer.h"




// Bresenham, integer-only , all eight octants via error-term/sign approach
void fb_line(int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b)
{
    int dx = x1 - x0;
    int dy = y1 - y0;
    int abs_dx = dx < 0 ? -dx : dx;
    int abs_dy = dy < 0 ? -dy : dy;
    int sx = dx < 0 ? -1 : 1;
    int sy = dy < 0 ? -1 : 1;
    int err = (abs_dx > abs_dy ? abs_dx : -abs_dy) / 2;

    int x = x0, y = y0;
    for (;;)
    {
        fb_put_pixel(x, y, r, g, b);
        if (x == x1 && y == y1)
            break;

        int e2 = err;
        if (e2 > -abs_dx)
        {
            err -= abs_dy;
            x += sx;
        }
        if (e2 < abs_dy)
        {
            err += abs_dx;
            y += sy;
        }
    }
}

void fb_rect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b)
{
    fb_line(x, y, x + w - 1, y, r, g, b);
    fb_line(x, y + h - 1, x + w - 1, y + h - 1, r, g, b);
    fb_line(x, y, x, y + h - 1, r, g, b);
    fb_line(x + w - 1, y, x + w - 1, y + h - 1, r, g, b);
}

void fb_rect_filled(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b)
{
    for (int row = 0; row < h; row++)
        fb_line(x, y + row, x + w - 1, y + row, r, g, b);
}

// Midpoint circle algorithm, eight-way symmetry
void fb_circle(int cx, int cy, int radius, uint8_t r, uint8_t g, uint8_t b)
{
    int x = radius;
    int y = 0;
    int err = 0;

    while (x >= y)
    {
        fb_put_pixel(cx + x, cy + y, r, g, b);
        fb_put_pixel(cx + y, cy + x, r, g, b);
        fb_put_pixel(cx - y, cy + x, r, g, b);
        fb_put_pixel(cx - x, cy + y, r, g, b);
        fb_put_pixel(cx - x, cy - y, r, g, b);
        fb_put_pixel(cx - y, cy - x, r, g, b);
        fb_put_pixel(cx + y, cy - x, r, g, b);
        fb_put_pixel(cx + x, cy - y, r, g, b);

        y += 1;
        err += 1 + 2 * y;
        if (2 * err - 2 * x + 1 > 0)
        {
            x -= 1;
            err += 1 - 2 * x;
        }
    }
}

void fb_circle_filled(int cx, int cy, int radius, uint8_t r, uint8_t g, uint8_t b)
{
    for (int dy = -radius; dy <= radius; dy++)
    {
        int dx = radius * radius - dy * dy;
        int span = 0;
        while (span * span <= dx)
            span++;
        span--;
        fb_line(cx - span, cy + dy, cx + span, cy + dy, r, g, b);
    }
}