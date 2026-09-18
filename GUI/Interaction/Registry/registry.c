#include "registry.h"
#include "../../../Lib/string.h"

extern uint64_t get_ticks(void);

static registry_entry_t entries[REGISTRY_MAX_ENTRIES];
static registry_history_entry_t history[REGISTRY_HISTORY_LEN];

static uint32_t history_head = 0;

static uint32_t history_count = 0; 


static void push_history(const char *name, entry_state_t from, entry_state_t to, uint64_t tick)
{
    registry_history_entry_t *h = &history[history_head];
    strncpy(h->name, name, REGISTRY_NAME_LEN);

    h->from_state = from;
    h->to_state = to;

    h->tick = tick;

    history_head = (history_head + 1) % REGISTRY_HISTORY_LEN;

    if (history_count < REGISTRY_HISTORY_LEN)
        history_count++;
}

static registry_entry_t *find(const char *name)
{
    for (int i = 0; i < REGISTRY_MAX_ENTRIES; i++)
    {
        if (entries[i].in_use && strcmp(entries[i].name, name) == 0)
            return &entries[i];
    }
    return NULL;
}

static registry_entry_t *find_free_slot(void)
{
    for (int i = 0; i < REGISTRY_MAX_ENTRIES; i++)
    {
        if (!entries[i].in_use)
            return &entries[i];
    }
    return NULL;
}

static void transition(registry_entry_t *e, entry_state_t to)
{
    entry_state_t from = e->state;

    if (from == to)
        return;

    e->state = to;

    push_history(e->name, from, to, get_ticks());
}

void registry_init(void)
{
    for (int i = 0; i < REGISTRY_MAX_ENTRIES; i++)
        entries[i].in_use = false;

    history_head = 0;
    history_count = 0;
}

bool registry_register(const char *name)
{
    if (find(name))
        return true; // already exists

    registry_entry_t *slot = find_free_slot();
    if (!slot)
        return false; // registry full

    strncpy(slot->name, name, REGISTRY_NAME_LEN);
    slot->state = ENTRY_NOT_RUNNING;

    slot->last_heartbeat_tick = 0;
    slot->launch_tick = 0;

    slot->in_use = true;
    return true;
}

bool registry_mark_launched(const char *name)
{
    registry_entry_t *e = find(name);
    if (!e)
        return false;

    e->launch_tick = get_ticks();

    transition(e, ENTRY_RUNNING_UNVERIFIED);
    return true;
}

bool registry_heartbeat(const char *name)
{
    registry_entry_t *e = find(name);
    if (!e)
        return false;

    e->last_heartbeat_tick = get_ticks();
    transition(e, ENTRY_RUNNING_VERIFIED);
    return true;
}

bool registry_mark_stopped(const char *name)
{
    registry_entry_t *e = find(name);
    if (!e)
        return false;

    transition(e, ENTRY_NOT_RUNNING);
    return true;
}

entry_state_t registry_query(const char *name)
{
    registry_entry_t *e = find(name);
    return e ? e->state : ENTRY_NOT_RUNNING;
}

uint32_t registry_refresh_staleness(uint64_t now, uint64_t stale_after_ticks)
{
    uint32_t changed = 0;

    for (int i = 0; i < REGISTRY_MAX_ENTRIES; i++)
    {
        registry_entry_t *e = &entries[i];

        if (!e->in_use || e->state != ENTRY_RUNNING_VERIFIED)
            continue;

        if (now - e->last_heartbeat_tick > stale_after_ticks)
        {
            transition(e, ENTRY_RUNNING_UNVERIFIED);

            changed++;
        }
    }
    return changed;
}

uint32_t registry_list(registry_entry_t *out, uint32_t max_entries)
{
    uint32_t n = 0;

    for (int i = 0; i < REGISTRY_MAX_ENTRIES && n < max_entries; i++)
    {

        if (entries[i].in_use)
            out[n++] = entries[i];
    }
    return n;
}

uint32_t registry_get_history(registry_history_entry_t *out, uint32_t max_entries)
{
    uint32_t n = history_count < max_entries ? history_count : max_entries;
    uint32_t oldest = (history_head + REGISTRY_HISTORY_LEN - history_count) % REGISTRY_HISTORY_LEN;

    for (uint32_t i = 0; i < n; i++)
        out[i] = history[(oldest + i) % REGISTRY_HISTORY_LEN];
    return n;
}

const char *registry_dependency_note(void)
{
    return "The unified running/launch list has nothing to show without this .... it's the source of truth the UI reads";
}

bool registry_selftest(void)
{
    registry_init();

    if (!registry_register("terminal"))
        return false;

    if (registry_query("terminal") != ENTRY_NOT_RUNNING)
        return false;

    if (!registry_mark_launched("terminal"))
        return false;

    if (registry_query("terminal") != ENTRY_RUNNING_UNVERIFIED)
        return false;

    if (!registry_heartbeat("terminal"))
        return false;

    if (registry_query("terminal") != ENTRY_RUNNING_VERIFIED)
        return false;

    if (registry_heartbeat("ghost") != false)
        return false;
    if (registry_query("ghost") != ENTRY_NOT_RUNNING)
        return false;

    uint32_t before = history_count;

    registry_heartbeat("terminal");
    if (history_count != before)
        return false;

    if (!registry_mark_stopped("terminal"))
        return false;

    if (registry_query("terminal") != ENTRY_NOT_RUNNING)
        return false;

    registry_history_entry_t hist[REGISTRY_HISTORY_LEN];

    uint32_t n = registry_get_history(hist, REGISTRY_HISTORY_LEN);
    if (n != 3)
        return false;

    if (hist[0].to_state != ENTRY_RUNNING_UNVERIFIED)
        return false;
    if (hist[1].to_state != ENTRY_RUNNING_VERIFIED)
        return false;

    if (hist[2].to_state != ENTRY_NOT_RUNNING)
        return false;

    return true;

}
