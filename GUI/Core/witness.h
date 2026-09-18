#ifndef WITNESS_H
#define WITNESS_H

#include <stdint.h>
#include <stdbool.h>
#include "registry.h"

#define WITNESS_MAX_TARGETS  32
#define WITNESS_LOG_LEN      16

typedef enum {
    INTENT_NONE,     // click landed on nothing
    INTENT_LAUNCH,
    INTENT_FOCUS
} intent_t;

typedef struct {
    char    name[REGISTRY_NAME_LEN];
    int32_t x, y, w, h;
    bool    in_use;
} witness_target_t;

typedef struct {
    char     name[REGISTRY_NAME_LEN];
    intent_t intent;
    entry_state_t state_seen;   // registry state witness based its call on
    uint64_t tick;
    
} witness_log_entry_t;

typedef struct {
    char     name[REGISTRY_NAME_LEN];

    intent_t intent;
    bool     hit;   // false if the click landed on nothing

} witness_result_t;

void witness_init(void);

bool witness_register_target(const char *name, int32_t x, int32_t y, int32_t w, int32_t h);

void witness_clear_targets(void);

witness_result_t witness_resolve_click(int32_t x, int32_t y);

uint32_t witness_get_log(witness_log_entry_t *out, uint32_t max_entries);

const char *witness_dependency_note(void);

bool witness_selftest(void);

#endif