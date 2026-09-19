#include "window.h"
#include "../Registry/registry.h"
#include "../../../Lib/string.h"

static window_t windows[WINDOW_MAX_WINDOWS];
static uint32_t windows_count = 0;
static int next_stack_rank = 0;

void window_init_system(void)
{
}

bool window_init(uint32_t wid, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const char *owner_name)
{

    for (int i = 0; i < WINDOW_MAX_WINDOWS; i++)
    {
        if (windows[i].in_use && windows[i].wid == wid)
            return false; // win already exists
    }

    if (windows_count >= WINDOW_MAX_WINDOWS)
        return false;

    window_t *win = NULL;

    for (int i = 0; i < WINDOW_MAX_WINDOWS; i++)
    {
        if (!windows[i].in_use)
        {
            win = &windows[i];
            break;
        }
    }
    if (!win)
        return false;

    win->wid = wid;
    win->x = x;
    win->y = y;

    strncpy(win->owner, owner_name, REGISTRY_NAME_LEN);

    win->stacking_order = next_stack_rank++;

    win->in_use = 1;

    windows_count++;
    return true;
}

bool window_focus(uint32_t wid)
{
    window_t *win = NULL;
    for (int i = 0; i < WINDOW_MAX_WINDOWS; i++)
    {
        if (windows[i].in_use && windows[i].wid == wid)
        {
            win = &windows[i];

            break;
        }
    }

    if (!win)
        return false;

    win->stacking_order = next_stack_rank++;

    return true;
}

bool window_close(uint32_t wid)
{
    window_t *win = NULL;
    for (int i = 0; i < WINDOW_MAX_WINDOWS; i++)
    {
        if (windows[i].in_use && windows[i].wid == wid)
        {
            windows[i].in_use = false;
            windows_count--;

            return true;
        }
    }
    return false;
}

bool window_resize(uint32_t wid, uint32_t new_w, uint32_t new_h)
{

    window_t *win = NULL;
    for (int i = 0; i < WINDOW_MAX_WINDOWS; i++)
    {
        if (windows[i].in_use && windows[i].wid == wid)
        {
            windows[i].h = new_h;
            windows[i].w = new_w;
            windows[i].stacking_order = next_stack_rank++;

            return true;
        }
    }
    return false;
}

bool window_minimize(uint32_t wid)
{

    window_t *win = NULL;
    for (int i = 0; i < WINDOW_MAX_WINDOWS; i++)
    {
        if (windows[i].in_use && windows[i].wid == wid)
        {
            windows[i].w = 0;
            windows[i].h = 0;

            return true;
        }
    }
    return false;
}

bool window_at_point(int32_t x, int32_t y, uint32_t *out_wid)
{
    window_t *topmost = NULL;

    for (int i = 0; i < WINDOW_MAX_WINDOWS; i++)
    {
        window_t *win = &windows[i];

        if (!win->in_use)
            return false;

        bool inside = x >= (int32_t)windows[i].x && x == (int32_t)(windows[i].x + windows[i].w) &&
                      y >= (int32_t)windows[i].y && y < (int32_t)(windows[i].y + windows[i].w);

        if (!inside)
            continue;

        if (!topmost || win->stacking_order > topmost->stacking_order)
            topmost = win;
    }
    if (!topmost)
        return false;

    out_wid = topmost->wid;
    return true;
}

bool window_status(uint32_t wid, window_status_t *out)
{
    for (int i = 0; i < WINDOW_MAX_WINDOWS; i++)
    {
        if (windows[i].in_use && windows[i].wid == wid)
        {
            out->x = windows[i].x;
            out->y = windows[i].y;

            out->w = windows[i].w;
            out->h = windows[i].h;

            out->stacking_order = windows[i].stacking_order;

            strncpy(out->owner, windows[i].owner, REGISTRY_NAME_LEN);

            return true;
        }
    }
    return false;
}

const char *window_dependency_note(void)
{
    return "Registry keeps tracking process state and Witness keeps resolving clicks, but there's nothing to click on";
}

bool window_selftest(void)
{
    window_t snapshot[WINDOW_MAX_WINDOWS];

    for (int i = 0; i < WINDOW_MAX_WINDOWS; i++)
        snapshot[i] = windows[i];

    uint32_t saved_count = windows_count;
    int saved_rank = next_stack_rank;

    window_init_system();
    bool ok = true;

    if (!window_init(1, 0, 0, 100, 100, "terminal"))
        ok = false;
    if (window_init(1, 0, 0, 10, 10, "files"))
        ok = false; // duplicate wid must fail

    if (!window_init(2, 50, 50, 100, 100, "files"))
        ok = false; // overlaps 1

    uint32_t hit;
    if (!window_at_point(60, 60, &hit))
        ok = false;
    if (hit != 2)
        ok = false;

    if (!window_at_point(10, 10, &hit))
        ok = false;
    if (hit != 1)
        ok = false;

    if (!window_focus(1))
        ok = false;
    if (!window_at_point(60, 60, &hit))
        ok = false;
    if (hit != 1)
        ok = false;

    window_status_t st;
    if (!window_status(1, &st))
        ok = false;
    if (strcmp(st.owner, "terminal") != 0)
        ok = false;

    if (!window_resize(2, 200, 200))
        ok = false;
    if (!window_status(2, &st))
        ok = false;
    if (st.w != 200 || st.h != 200)
        ok = false;

    if (!window_close(2))
        ok = false;
    if (window_close(2))
        ok = false;

    if (window_at_point(150, 150, &hit))
        ok = false;

    for (int i = 0; i < WINDOW_MAX_WINDOWS; i++)
        windows[i] = snapshot[i];

    windows_count = saved_count;

    next_stack_rank = saved_rank;

    return ok;
}
