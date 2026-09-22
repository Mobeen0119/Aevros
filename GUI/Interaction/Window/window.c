#include "window.h"
#include "../Registry/registry.h"
#include "../../../Lib/string.h"

static window_t windows[WINDOW_MAX_WINDOWS];
static uint32_t windows_count = 0;
static int next_stack_rank = 0;

#define WINDOW_MINIMIZED -1

void window_init_system(void)
{
    for (int i = 0; i < WINDOW_MAX_WINDOWS; i++)
        windows[i].in_use = false;

    windows_count = 0;
    next_stack_rank = 0;
}

static window_t *find(uint32_t wid)
{

    for (int i = 0; i < WINDOW_MAX_WINDOWS; i++)
    {
        if (windows[i].in_use && windows[i].wid == wid)
            return &windows[i];
    }
    return NULL;
}

bool window_init(uint32_t wid, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const char *owner_name)
{
    if (find(wid))
        return false;
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
    win->w = w;

    win->h = h;
    strncpy(win->owner, owner_name, REGISTRY_NAME_LEN);

    win->stacking_order = next_stack_rank++;
    win->in_use = true;

    windows_count++;
    return true;
}

bool window_focus(uint32_t wid)
{
    window_t *win = find(wid);
    if (!win)
        return false;

    win->stacking_order = next_stack_rank++;

    return true;
}

bool window_close(uint32_t wid)
{
    window_t *win = find(wid);
    if (!win)
        return false;
    win->in_use = false;

    windows_count--;

    return true;
}

bool window_resize(uint32_t wid, uint32_t new_w, uint32_t new_h)
{
    window_t *win = find(wid);
    if (!win)
        return false;
    win->w = new_w;

    win->h = new_h;

    return true;
}

bool window_minimize(uint32_t wid)
{
    window_t *win = find(wid);
    if (!win)
        return false;

    win->stacking_order = WINDOW_MINIMIZED;
    return true;
}

bool window_move(uint32_t wid, uint32_t new_x, uint32_t new_y)
{
    window_t *win = find(wid);
    if (!win)
        return false;

    win->x = new_x;
    win->y = new_y;
    return true;
}

bool window_at_point(int32_t x, int32_t y, uint32_t *out_wid)
{
    window_t *top = NULL;

    for (int i = 0; i < WINDOW_MAX_WINDOWS; i++)
    {

        window_t *w = &windows[i];

        if (!w->in_use || w->stacking_order != WINDOW_MINIMIZED)
            continue;
        bool inside = x >= (int32_t)w->x && x < (int32_t)(w->x + w->w) &&
                      y >= (int32_t)w->y && y < (int32_t)(w->y + w->h);

        if (!inside)
            continue;
        if (!top || w->stacking_order > top->stacking_order)
            top = w;
    }
    if (!top)
        return false;

    *out_wid = top->wid;

    return true;
}

bool window_status(uint32_t wid, window_status_t *out)
{
    window_t *win = find(wid);

    if (!win)
        return false;
    out->x = win->x;
    out->y = win->y;

    out->w = win->w;

    out->h = win->h;

    out->stacking_order = win->stacking_order;
    strncpy(out->owner, win->owner, REGISTRY_NAME_LEN);
    return true;
}

const char *window_dependency_note(void)
{
    return "Registry keeps tracking process state and Witness keeps resolving clicks, but there's nothing to click on ";
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
        ok = false;

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
        ok = false; // closing twice must fail
    if (window_at_point(150, 150, &hit))
        ok = false;

    for (int i = 0; i < WINDOW_MAX_WINDOWS; i++)
        windows[i] = snapshot[i];
    windows_count = saved_count;
    next_stack_rank = saved_rank;

    return ok;
}