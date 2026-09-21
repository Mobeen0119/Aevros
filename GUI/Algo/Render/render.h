#ifndef RENDER_H
#define RENDER_H

#include <stdint.h>
#include <stdbool.h>

void window_render(uint32_t wid);

void render_all_windows(void);

const char *render_dependency_note(void);
bool render_selftest(void);

#endif