#include "mouse_bridge.h"
#include "../../../Drivers/Mouse/mouse.h"
#include "../../Algo/Cursor/cursor.h"
#include "../../Core/witness.h"
#include "../Window/window.h"
#include "../Registry/registry.h"

extern uint64_t get_ticks(void);

static uint64_t events_forwarded = 0;
static uint64_t clicks_resolved = 0;
static uint64_t last_pump_tick = 0;
static bool last_left_button_state = false;

void bridge_init(int32_t start_x, int32_t start_y)
{
    cursor_init(start_x, start_y);
    events_forwarded = 0;
    clicks_resolved = 0;
    last_left_button_state = false;
    last_pump_tick = get_ticks();
}

static void handle_click(int32_t x, int32_t y)
{
    witness_result_t result = witness_resolve_click(x, y);
    if (!result.hit)
        return;

    if (result.intent == INTENT_FOCUS)
    {
        window_focus(result.wid);
    }
    else if (result.intent == INTENT_LAUNCH)
    {

        registry_mark_launched(result.name);
        window_focus(result.wid);
    }
}

void bridge_pump(void)
{
    mouse_event_t ev;
    while (ps2_mouse_poll_event(&ev))
    {
        cursor_update(ev);

        if (ev.left_button && !last_left_button_state)
        {
            cursor_status_t cs;
            cursor_status(&cs);

            handle_click(cs.x, cs.y);

            clicks_resolved++;
        }
        last_left_button_state = ev.left_button;

        events_forwarded++;
    }
    last_pump_tick = get_ticks();
}

void bridge_status(bridge_status_t *out)
{
    out->events_forwarded = events_forwarded;
    out->clicks_resolved = clicks_resolved;
    out->last_pump_tick = last_pump_tick;
}

const char *bridge_dependency_note(void)
{
    return "Mouse driver keeps queuing packets and the cursor holds its last position if stops running ... nothing crashes, the pointer just freezes and clicks stop resolving.";
}

bool bridge_selftest(void)
{
    registry_init();
    window_init_system();
    witness_init();

    registry_register("terminal");
   
    window_init(1, 10, 10, 50, 50, "terminal");
   
    registry_register("files");
    window_init(2, 200, 10, 50, 50, "files");

    if (registry_query("terminal") != ENTRY_NOT_RUNNING)
        return false;

    handle_click(20, 20);
    if (registry_query("terminal") != ENTRY_RUNNING_UNVERIFIED)
        return false;

    window_status_t st;
    if (!window_status(1, &st))
        return false;

    int focused_rank = st.stacking_order;

    if (!window_status(2, &st))
        return false;
    if (focused_rank <= st.stacking_order)
        return false; // window 1 should be on top of window 2 in test

    registry_heartbeat("terminal");
    if (registry_query("terminal") != ENTRY_RUNNING_VERIFIED)
        return false;

    window_status_t st1, st2;
    handle_click(200, 20);

    handle_click(20, 20);

    if (!window_status(1, &st1))
        return false;

    if (!window_status(2, &st2))
        return false;

    if (st1.stacking_order <= st2.stacking_order)
        return false;
    if (registry_query("terminal") != ENTRY_RUNNING_VERIFIED)
        return false;

    uint64_t before = clicks_resolved;

    handle_click(999, 999);

    if (clicks_resolved != before)
        return false;

    return true;
}
