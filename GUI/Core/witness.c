#include "witness.h"
#include "../../Lib/string.h"

extern uint64_t get_ticks(void);

static witness_target_t targets[WITNESS_MAX_TARGETS];
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

static bool point_in_rect(int32_t px, int32_t py, witness_target_t *t)
{
    return px >= t->x && px < t->x + t->w && py >= t->y && py < t->y + t->h;
}

static intent_t intent_for_state(entry_state_t state)
{
    return (state == ENTRY_NOT_RUNNING) ? INTENT_LAUNCH : INTENT_FOCUS;
}

void witness_init(void)
{
    for (int i = 0; i < WITNESS_MAX_TARGETS; i++)
        targets[i].in_use = false;

    log_head = 0;
    log_count = 0;
}

bool witness_register_target(const char *name, int32_t x, int32_t y, int32_t w, int32_t h)
{
    witness_target_t *slot = NULL;
    for (int i = 0; i < WITNESS_MAX_TARGETS; i++)
    {
        if (targets[i].in_use && strcmp(targets[i].name, name) == 0)
        {
            slot = &targets[i];
            break;
        }
        if (!slot && !targets[i].in_use)
            slot = &targets[i];
    }
    if (!slot)
        return false;

    strncpy(slot->name, name, REGISTRY_NAME_LEN);
    slot->x = x;

    slot->y = y;

    slot->w = w;
    slot->h = h;

    slot->in_use = true;

    return true;
}

void witness_clear_targets(void)
{
    for (int i = 0; i < WITNESS_MAX_TARGETS; i++)
        targets[i].in_use = false;
}

witness_result_t witness_resolve_click(int32_t x, int32_t y)
{
    witness_result_t result = {.name = "", .intent = INTENT_NONE, .hit = false};

    for (int i = 0; i < WITNESS_MAX_TARGETS; i++)
    {
        if (!targets[i].in_use || !point_in_rect(x, y, &targets[i]))
            continue;

        entry_state_t state = registry_query(targets[i].name);
        intent_t intent = intent_for_state(state);

        strncpy(result.name, targets[i].name, REGISTRY_NAME_LEN);

        result.intent = intent;
        result.hit = true;

        push_log(targets[i].name, intent, state, get_ticks());
        return result;
    }

    push_log("", INTENT_NONE, ENTRY_NOT_RUNNING, get_ticks());
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

    witness_target_t box = {"", 10, 10, 20, 20, true};

    if (!point_in_rect(10, 10, &box))
        return false; // top-left inclusive

    if (point_in_rect(30, 10, &box))
        return false; // right edge exclusive

    if (point_in_rect(10, 30, &box))
        return false; // bottom edge exclusive

    if (!point_in_rect(29, 29, &box))
        return false; // inside

    witness_target_t targets_snapshot[WITNESS_MAX_TARGETS];

    witness_log_entry_t log_snapshot[WITNESS_LOG_LEN];

    for (int i = 0; i < WITNESS_MAX_TARGETS; i++)
        targets_snapshot[i] = targets[i];

    for (int i = 0; i < WITNESS_LOG_LEN; i++)
        log_snapshot[i] = log_buf[i];
    uint32_t saved_log_head = log_head, saved_log_count = log_count;

    bool ok = true;

    witness_clear_targets();

    if (!witness_register_target("alpha", 0, 0, 5, 5))
        ok = false;

    if (!witness_register_target("alpha", 1, 1, 6, 6))
        ok = false; // overwrite

    bool found_overwritten = false;

    for (int i = 0; i < WITNESS_MAX_TARGETS; i++)

    {
        if (targets[i].in_use && strcmp(targets[i].name, "alpha") == 0)
        {
            found_overwritten = (targets[i].x == 1);
        }
    }
    if (!found_overwritten)
        ok = false;

    char name_buf[REGISTRY_NAME_LEN];
    for (int i = 0; i < WITNESS_MAX_TARGETS - 1; i++)
    {
        name_buf[0] = (char)('b' + (i / 26)); // unique pair per i
        name_buf[1] = (char)('a' + (i % 26));
        name_buf[2] = '\0';
        if (!witness_register_target(name_buf, i, i, 1, 1))
            ok = false;
    }
    if (witness_register_target("overflow", 0, 0, 1, 1))
        ok = false; // must fail, table is full

    log_head = 0;
    log_count = 0;
    for (int i = 0; i < WITNESS_LOG_LEN + 3; i++)
    {
        push_log("x", INTENT_FOCUS, ENTRY_RUNNING_VERIFIED, (uint64_t)i);
    }
    if (log_count != WITNESS_LOG_LEN)
        ok = false;

    for (int i = 0; i < WITNESS_MAX_TARGETS; i++)
        targets[i] = targets_snapshot[i];
    for (int i = 0; i < WITNESS_LOG_LEN; i++)
        log_buf[i] = log_snapshot[i];

    log_head = saved_log_head;
    log_count = saved_log_count;

    return ok;
}