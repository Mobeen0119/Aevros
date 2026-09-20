#include "mouse_bridge.h"
#include "../../../Drivers/Mouse/mouse.h"
#include "../../Algo/Cursor/cursor.h"
#include "../../Core/witness.h"

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
            witness_result_t result = witness_resolve_click(cs.x, cs.y);

            if (result.hit && result.intent == INTENT_FOCUS)
            {
                window_focus(result.wid);
            }

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