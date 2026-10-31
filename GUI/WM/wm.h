#ifndef WM_H
#define WM_H

#include <stdint.h>
#include <stdbool.h>

typedef void (*wm_paint_fn)(int x, int y, int w, int h);
typedef const char *(*wm_status_fn)(uint32_t *color);
typedef void (*wm_why_fn)(char *buf, int cap);

void wm_init(void);

int wm_open(const char *title, int x, int y, int w, int h, wm_paint_fn paint, wm_status_fn status, wm_why_fn why);

void wm_paint_all(void);

bool wm_mouse(int32_t mx, int32_t my, bool down, bool pressed, bool released);

#endif