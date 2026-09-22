#include "render.h"
#include "../../Interaction/Window/window.h"
#include "../../Interaction/Registry/registry.h"
#include "../Font/font.h"

extern void fb_put_pixel(int32_t x, int32_t y, uint32_t color);
extern uint64_t get_ticks(void);

#define COLOR_VERIFIED 0x6FA3C2
#define COLOR_UNVERIFIED 0x7C7E82

#define COLOR_ORPHANED 0xB8483F
#define COLOR_TITLE_TEXT 0xE6E5E2

#define MAX_DAMAGE_RECTS WINDOW_MAX_WINDOWS
static uint32_t bg_color = 0x000000;

typedef struct
{
    int32_t x, y;

    uint32_t w, h;
} rect_t;

typedef struct
{
    uint32_t wid;
    entry_state_t last_state;

    int last_stacking_order;
    uint32_t last_x, last_y, last_w, last_h;
    bool has_cache;
} render_cache_t;

static render_cache_t cache[WINDOW_MAX_WINDOWS];
static render_log_entry_t render_log[RENDER_LOG_LEN];
static uint32_t log_head = 0;

static uint32_t log_count = 0;

static bool rects_intersect(rect_t a, rect_t b)
{
    return a.x < (int32_t)(b.x + b.w) && b.x < (int32_t)(a.x + a.w) &&
           a.y < (int32_t)(b.y + b.h) && b.y < (int32_t)(a.y + a.h);
}

static void fill_rect(int32_t x, int32_t y, uint32_t w, uint32_t h, uint32_t color)
{
    for (uint32_t row = 0; row < h; row++)
        for (uint32_t col = 0; col < w; col++)
            fb_put_pixel(x + col, y + row, color);
}

static void push_render_log(uint32_t wid, redraw_reason_t reason, uint64_t tick)
{
    render_log[log_head] = (render_log_entry_t){wid, reason, tick};
    log_head = (log_head + 1) % RENDER_LOG_LEN;

    if (log_count < RENDER_LOG_LEN)
        log_count++;
}

static render_cache_t *find_cache(uint32_t wid)
{
    for (int i = 0; i < WINDOW_MAX_WINDOWS; i++)
    {

        if (cache[i].has_cache && cache[i].wid == wid)
            return &cache[i];
    }
    return NULL;
}

static render_cache_t *find_free_cache_slot(void)
{
    for (int i = 0; i < WINDOW_MAX_WINDOWS; i++)
    {
        if (!cache[i].has_cache)
            return &cache[i];
    }
    return NULL;
}

static uint32_t border_color_for(entry_state_t state)
{
    switch (state)
    {
    case ENTRY_RUNNING_VERIFIED:
        return COLOR_VERIFIED;

    case ENTRY_RUNNING_UNVERIFIED:
        return COLOR_UNVERIFIED;

    default:
        return COLOR_ORPHANED;
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

    draw_rect_outline(win.x, win.y, win.w, win.h, border_color_for(state));
    font_draw_string(win.x + 4, win.y + 4, win.owner, COLOR_TITLE_TEXT);
}

static bool needs_redraw(render_cache_t *c, const window_t *w, entry_state_t state, redraw_reason_t *out_reason)
{
    if (!c->has_cache)
    {
        *out_reason = REDRAW_NEW;
        return true;
    }
    if (c->last_state != state)
    {
        *out_reason = REDRAW_STATE_CHANGED;
        return true;
    }

    if (c->last_stacking_order != w->stacking_order)
    {
        *out_reason = REDRAW_STACK_CHANGED;
        return true;
    }

    if (c->last_x != w->x || c->last_y != w->y || c->last_w != w->w || c->last_h != w->h)
    {
        *out_reason = REDRAW_GEOMETRY_CHANGED;
        return true;
    }
    return false;
}

void render_all_windows(void)
{
    window_t all[WINDOW_MAX_WINDOWS];

    uint32_t n = window_list(all, WINDOW_MAX_WINDOWS);

    rect_t damage[MAX_DAMAGE_RECTS];

    uint32_t damage_count = 0;
    redraw_reason_t reasons[WINDOW_MAX_WINDOWS];
    bool must_redraw[WINDOW_MAX_WINDOWS] = {0};

    for (uint32_t i = 0; i < n; i++)
    {
        entry_state_t state = registry_query(all[i].owner);

        render_cache_t *c = find_cache(all[i].wid);

        bool is_new = !c;
        if (is_new)
            c = find_free_cache_slot();

        if (!c)
            continue;

        redraw_reason_t reason;
        bool changed = needs_redraw(c, &all[i], state, &reason);

        if (!is_new && reason == REDRAW_GEOMETRY_CHANGED && damage_count < MAX_DAMAGE_RECTS)
        {

            damage[damage_count++] = (rect_t){c->last_x, c->last_y, c->last_w, c->last_h};
        }

        if (changed)
        {
            must_redraw[i] = true;
            reasons[i] = reason;
        }
    }

    for (uint32_t d = 0; d < damage_count; d++)
        fill_rect(damage[d].x, damage[d].y, damage[d].w, damage[d].h, bg_color);

    for (uint32_t i = 0; i < n; i++)
    {
        rect_t r = {all[i].x, all[i].y, all[i].w, all[i].h};
        for (uint32_t d = 0; d < damage_count; d++)
            if (rects_intersect(r, damage[d]))
            {
                must_redraw[i] = true;
                break;
            }
    }

    bool drawn[WINDOW_MAX_WINDOWS] = {0};
    for (uint32_t done = 0; done < n; done++)
    {
        int best = -1;
        for (uint32_t i = 0; i < n; i++)
        {
            if (drawn[i])
                continue;
            if (best < 0 || all[i].stacking_order < all[best].stacking_order)
                best = i;
        }
        drawn[best] = true;

        window_t *w = &all[best];

        render_cache_t *c = find_cache(w->wid);
        bool is_new = !c;
        if (is_new)
            c = find_free_cache_slot();

        if (!c)
            continue;

        if (must_redraw[best])
        {
            window_render(w->wid);
            push_render_log(w->wid, is_new ? REDRAW_NEW : reasons[best], get_ticks());
        }

        entry_state_t state = registry_query(w->owner);

        c->wid = w->wid;
        c->last_state = state;
        c->last_stacking_order = w->stacking_order;
        c->last_x = w->x;
        c->last_y = w->y;

        c->last_w = w->w;
        c->last_h = w->h;
        c->has_cache = true;
    }
}

uint32_t render_get_log(render_log_entry_t *out, uint32_t max_entries)
{
    uint32_t n = log_count < max_entries ? log_count : max_entries;

    uint32_t oldest = (log_head + RENDER_LOG_LEN - log_count) % RENDER_LOG_LEN;

    for (uint32_t i = 0; i < n; i++)
        out[i] = render_log[(oldest + i) % RENDER_LOG_LEN];

    return n;
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

    render_cache_t c = {0};

    redraw_reason_t reason;

    if (!needs_redraw(&c, ENTRY_RUNNING_VERIFIED, 5, &reason))
        return false;

    if (reason != REDRAW_NEW)
        return false;

    c.has_cache = true;

    c.last_state = ENTRY_RUNNING_VERIFIED;

    c.last_stacking_order = 5;

    if (needs_redraw(&c, ENTRY_RUNNING_VERIFIED, 5, &reason))
        return false; // nothing changed

    if (!needs_redraw(&c, ENTRY_RUNNING_UNVERIFIED, 5, &reason))
        return false;
    if (reason != REDRAW_STATE_CHANGED)
        return false;

    c.last_state = ENTRY_RUNNING_UNVERIFIED;

    if (!needs_redraw(&c, ENTRY_RUNNING_UNVERIFIED, 9, &reason))
        return false;
    if (reason != REDRAW_STACK_CHANGED)
        return false;

    return true;
}
