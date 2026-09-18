#ifndef REGISTRY_H

#define REGISTRY_H

#include <stdint.h>
#include <stdbool.h>

#define REGISTRY_MAX_ENTRIES  32
#define REGISTRY_NAME_LEN     32
#define REGISTRY_HISTORY_LEN  32

typedef enum {
    ENTRY_NOT_RUNNING,

    ENTRY_RUNNING_UNVERIFIED,
    ENTRY_RUNNING_VERIFIED

} entry_state_t;

typedef struct {
    char          name[REGISTRY_NAME_LEN];

    entry_state_t state;
    uint64_t      last_heartbeat_tick;
    uint64_t      launch_tick;
    bool          in_use;

} registry_entry_t;

typedef struct {
    char          name[REGISTRY_NAME_LEN];
    entry_state_t from_state;

    entry_state_t to_state;

    uint64_t      tick;
} registry_history_entry_t;

void registry_init(void);

bool registry_register(const char *name);

bool registry_mark_launched(const char *name);

bool registry_heartbeat(const char *name);

bool registry_mark_stopped(const char *name);

entry_state_t registry_query(const char *name);

uint32_t registry_refresh_staleness(uint64_t now, uint64_t stale_after_ticks);

uint32_t registry_list(registry_entry_t *out, uint32_t max_entries);

uint32_t registry_get_history(registry_history_entry_t *out, uint32_t max_entries);

const char *registry_dependency_note(void);

bool registry_selftest(void);

#endif