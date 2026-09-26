#ifndef LIST_H
#define LIST_H

#include <stdint.h>
#include <stdbool.h>
#include "../Registry/registry.h"

#define LIST_MAX_ENTRIES REGISTRY_MAX_ENTRIES

typedef struct {
    char          name[REGISTRY_NAME_LEN];
    entry_state_t state;
    
    uint32_t      wid;
    bool          has_window;
} list_entry_t;

void list_init(int32_t origin_x, int32_t origin_y, uint32_t row_width, uint32_t row_height);

uint32_t list_get_entries(list_entry_t *out, uint32_t max_entries);

bool list_at_point(int32_t x, int32_t y, char *out_name);

const char *list_dependency_note(void);
bool list_selftest(void);

#endif