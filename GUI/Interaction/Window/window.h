#ifndef WINDOW_H
#define WINDOW_H

#include <stdint.h>
#include <stdbool.h>
#include "../Registry/registry.h"

#define WINDOW_MAX_WINDOWS 32

typedef struct {
    uint32_t x, y, w, h;
    int      stacking_order;
    uint32_t wid;
    char     owner[REGISTRY_NAME_LEN]; 
    bool     in_use;
} window_t;

typedef struct {
    uint32_t x, y, w, h;
    int      stacking_order;

    char     owner[REGISTRY_NAME_LEN];

} window_status_t;

void window_init_system(void);

bool window_init(uint32_t wid, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const char *owner_name);

bool window_focus(uint32_t wid);

bool window_close(uint32_t wid);
bool window_resize(uint32_t wid, uint32_t new_w, uint32_t new_h);

bool window_minimize(uint32_t wid);


bool window_at_point(int32_t x, int32_t y, uint32_t *out_wid);

bool window_status(uint32_t wid, window_status_t *out);
const char *window_dependency_note(void);

uint32_t window_list(window_t *out, uint32_t max_entries);

bool window_selftest(void);

#endif