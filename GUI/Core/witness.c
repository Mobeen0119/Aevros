#include "witness.h"
#include "../Interaction/Window/window.h"
#include "../../Lib/string.h"

extern uint64_t get_ticks(void);

static witness_log_entry_t log_buf[WITNESS_LOG_LEN];
static uint32_t log_head = 0;
static uint32_t log_count = 0;

static void push_log(const char *name, intent_t intent, entry_state_t state_seen, uint64_t tick)
{
    witness_log_entry_t *l = &log_buf[log_head];
    strncpy(l->name, name, REGISTRY_NAME_LEN);

    l->intent = intent;

    l->state_seen = state_seen;
    l->tick = tick;

    log_head = (log_head + 1) % WITNESS_LOG_LEN;
    if (log_count < WITNESS_LOG_LEN)
        log_count++;
}

static intent_t intent_for_state(entry_state_t state)
{
    return (state == ENTRY_NOT_RUNNING) ? INTENT_LAUNCH : INTENT_FOCUS;
}

void witness_init(void)
{
    log_head = 0;

    log_count = 0;
}

witness_result_t witness_resolve_click(int32_t x, int32_t y)
{
    witness_result_t result = {.name = "", .intent = INTENT_NONE, .hit = false};

    uint32_t wid;
    if (!window_at_point(x, y, &wid))
    {
        push_log("", INTENT_NONE, ENTRY_NOT_RUNNING, get_ticks());
        return result;
    }

    window_status_t win;
    if (!window_status(wid, &win))
    {
        push_log("", INTENT_NONE, ENTRY_NOT_RUNNING, get_ticks());
        return result;
    }

    entry_state_t state = registry_query(win.owner);
    intent_t intent = intent_for_state(state);

    strncpy(result.name, win.owner, REGISTRY_NAME_LEN);

    result.wid = wid;

    result.intent = intent;
    result.hit = true;

    push_log(win.owner, intent, state, get_ticks());
    return result;
}

uint32_t witness_get_log(witness_log_entry_t *out, uint32_t max_entries)
{
    uint32_t n = log_count < max_entries ? log_count : max_entries;

    uint32_t oldest = (log_head + WITNESS_LOG_LEN - log_count) % WITNESS_LOG_LEN;

    for (uint32_t i = 0; i < n; i++)
        out[i] = log_buf[(oldest + i) % WITNESS_LOG_LEN];
    return n;
}

const char *witness_dependency_note(void)
{
    return "Clicks still land without this, but nothing decides what they mean ... launch and focus become undefined actions.";
}

bool witness_selftest(void)
{
    if (intent_for_state(ENTRY_NOT_RUNNING) != INTENT_LAUNCH)
        return false;
    if (intent_for_state(ENTRY_RUNNING_UNVERIFIED) != INTENT_FOCUS)
        return false;
    if (intent_for_state(ENTRY_RUNNING_VERIFIED) != INTENT_FOCUS)
        return false;

    registry_init();

    window_init_system();
    witness_init();

    registry_register("terminal");
    window_init(1, 10, 10, 50, 50, "terminal");

    witness_result_t miss = witness_resolve_click(500, 500);
    if (miss.hit)
        return false;
    if (miss.intent != INTENT_NONE)
        return false;

    witness_result_t launch = witness_resolve_click(20, 20);
    if (!launch.hit)
        return false;

    if (launch.intent != INTENT_LAUNCH)
        return false; // registered but never marked launched
    if (strcmp(launch.name, "terminal") != 0)
        return false;

    registry_mark_launched("terminal");
    registry_heartbeat("terminal");

    witness_result_t focus = witness_resolve_click(20, 20);
    if (focus.intent != INTENT_FOCUS)
        return false;

    witness_log_entry_t hist[WITNESS_LOG_LEN];

    uint32_t n = witness_get_log(hist, WITNESS_LOG_LEN);
    if (n != 3)
        return false;
    if (hist[0].intent != INTENT_NONE)
        return false;
    if (hist[1].intent != INTENT_LAUNCH)
        return false;

    if (hist[2].intent != INTENT_FOCUS)
        return false;

    return true;
}