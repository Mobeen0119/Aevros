#ifndef WITNESS_H
#define WITNESS_H

#include <stdint.h>

#include <stdbool.h>
#include "../Interaction/Registry/registry.h"


#define WITNESS_LOG_LEN 16

typedef enum {
    INTENT_NONE,
    INTENT_LAUNCH,
    INTENT_FOCUS
} intent_t;

typedef struct {
    char name[REGISTRY_NAME_LEN];
    intent_t intent;

    entry_state_t state_seen;
    uint64_t tick;

} witness_log_entry_t;

typedef struct {
    char name[REGISTRY_NAME_LEN];
    intent_t intent;

    uint32_t wid;
    bool hit;

} witness_result_t;

void witness_init(void);

witness_result_t witness_resolve_click(int32_t x, int32_t y);

uint32_t witness_get_log(witness_log_entry_t *out, uint32_t max_entries);

const char *witness_dependency_note(void);

bool witness_selftest(void);

#endif