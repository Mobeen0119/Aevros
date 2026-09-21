#include "render.h"
#include "window.h"

#include "registry.h"
#include "font.h"

extern void fb_put_pixel(int32_t x, int32_t y, uint32_t color);

#define COLOR_VERIFIED 0x6FA3C2
#define COLOR_UNVERIFIED 0x7C7E82

#define COLOR_ORPHANED 0xB8483F // owner no longer exists in registry
#define COLOR_TITLE_TEXT 0xE6E5E2

static uint32_t border_color_for(entry_state_t state)
{
    switch (state)
    {
    case ENTRY_RUNNING_VERIFIED:
        return COLOR_VERIFIED;

    case ENTRY_RUNNING_UNVERIFIED:
        return COLOR_UNVERIFIED;
    default:
        return COLOR_ORPHANED; // NOT_RUNNING with a window still open
    }
}

static void draw_rect_outline(int32_t x, int32_t y, uint32_t w, uint32_t h, uint32_t color)
{
    for (uint32_t col = 0; col < w; col++)
    {

        fb_put_pixel(x + col, y, color);
        fb_put_pixel(x + col, y + h - 1, color);
    }
    for (uint32_t row = 0; row < h; row++)
    {
        fb_put_pixel(x, y + row, color);
        fb_put_pixel(x + w - 1, y + row, color);
    }
}

void window_render(uint32_t wid)
{
    window_status_t win;
    if (!window_status(wid, &win))
        return; // wid doesn't exist

    entry_state_t state = registry_query(win.owner);
    uint32_t border = border_color_for(state);

    draw_rect_outline(win.x, win.y, win.w, win.h, border);

    font_draw_string(win.x + 4, win.y + 4, win.owner, COLOR_TITLE_TEXT);
}

void render_all_windows(void)
{
    window_t all[WINDOW_MAX_WINDOWS];
    uint32_t n = window_list(all, WINDOW_MAX_WINDOWS);
    bool drawn[WINDOW_MAX_WINDOWS] = {0};
    for (uint32_t drawn_count = 0; drawn_count < n; drawn_count++)
    {
        int best = -1;

        for (uint32_t i = 0; i < n; i++)
        {
            if (drawn[i])
                continue;

            if (best < 0 || all[i].stacking_order < all[best].stacking_order)
                best = i;
        }
        window_render(all[best].wid);

        drawn[best] = true;
    }
}

const char *render_dependency_note(void)
{
    return "Registry, Witness, and Window keep tracking correct state, but the screen stays blank nothing else in this codebase turns that state into pixels.";
}

bool render_selftest(void)
{
    if (border_color_for(ENTRY_RUNNING_VERIFIED) != COLOR_VERIFIED)
        return false;
    if (border_color_for(ENTRY_RUNNING_UNVERIFIED) != COLOR_UNVERIFIED)
        return false;

    if (border_color_for(ENTRY_NOT_RUNNING) != COLOR_ORPHANED)
        return false;

    return true;
}