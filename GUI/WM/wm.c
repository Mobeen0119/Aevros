#include "wm.h"
#include "style.h"
#include "../../Lib/string.h"

#define WM_MAX 8
#define SCREEN_W 1024
#define SCREEN_H 768
#define CLOSE_HIT 18
#define WHY_CAP 128

typedef struct
{
    int x, y, w, h;
    char title[32];

    wm_paint_fn paint;
    wm_status_fn status;
    wm_why_fn why;

    bool open;
} window_t;

static window_t windows[WM_MAX];
static int window_count;
static int drag_index = -1;
static int drag_dx, drag_dy;

void wm_init(void)
{
    window_count = 0;
    drag_index = -1;
}

int wm_open(const char *title, int x, int y, int w, int h,
            wm_paint_fn paint, wm_status_fn status, wm_why_fn why)
{
    if (window_count >= WM_MAX)
        return -1;

    window_t *win = &windows[window_count];
    win->x = x;
    win->y = y;
    win->w = w;
    win->h = h;

    win->paint = paint;
    win->status = status;
    win->why = why;
    win->open = true;

    strncpy(win->title, title, sizeof(win->title) - 1);
    win->title[sizeof(win->title) - 1] = 0;

    return window_count++;
}

static int hit_test(int32_t mx, int32_t my)
{
    for (int i = window_count - 1; i >= 0; i--)
    {
        window_t *w = &windows[i];
        if (w->open && mx >= w->x && mx < w->x + w->w && my >= w->y && my < w->y + w->h)
            return i;
    }
    return -1;
}

static int raise_window(int i)
{
    window_t top = windows[i];

    for (int k = i; k < window_count - 1; k++)
        windows[k] = windows[k + 1];

    windows[window_count - 1] = top;
    return window_count - 1;
}

static void paint_window(window_t *w, bool focused)
{
    aev_panel(w->x, w->y, w->w, w->h, focused);

    aev_fill(w->x + 12, w->y + 10, 6, 6, AEV_DAWN);
    aev_text(w->x + 26, w->y + 9, w->title, AEV_TEXT, 1);

    int close_x = w->x + w->w - CLOSE_HIT;
    aev_cross(close_x + 4, w->y + 9, 7, AEV_MUTED);

    if (w->status)
    {
        uint32_t color = AEV_TEXT;
        const char *word = w->status(&color);

        int wx = close_x - 12 - (int)aev_text_width(word, 1);
        aev_text(wx, w->y + 9, word, color, 1);
    }

    char why[WHY_CAP];
    int len = 0;
    why[0] = 0;

    if (w->why)
        w->why(why, WHY_CAP);

    aev_text(w->x + 12, w->y + w->h - 15, "WHY", AEV_MUTED, 1);
    aev_text(w->x + 12 + (int)aev_text_width("WHY  ", 1), w->y + w->h - 15, why, AEV_TEXT, 1);

    (void)len;

    if (w->paint)
        w->paint(w->x + 1, w->y + AEV_HEAD_H + 1, w->w - 2, w->h - AEV_HEAD_H - AEV_FOOT_H - 2);
}

void wm_paint_all(void)
{
    for (int i = 0; i < window_count; i++)
    {
        if (windows[i].open)
            paint_window(&windows[i], i == window_count - 1);
    }
}

bool wm_mouse(int32_t mx, int32_t my, bool down, bool pressed, bool released)
{
    if (pressed)
    {
        int idx = hit_test(mx, my);
        if (idx < 0)
            return false;

        window_t *w = &windows[idx];
        bool in_head = my < w->y + AEV_HEAD_H;

        if (in_head && mx >= w->x + w->w - CLOSE_HIT)
        {
            w->open = false;
            return true;
        }

        int top = raise_window(idx);
        if (my < windows[top].y + AEV_HEAD_H)
        {
            drag_index = top;
            drag_dx = mx - windows[top].x;
            drag_dy = my - windows[top].y;
        }
        return true;
    }

    if (released)
    {
        drag_index = -1;
        return false;
    }

    if (down && drag_index >= 0)
    {
        window_t *w = &windows[drag_index];
        int nx = mx - drag_dx;
        int ny = my - drag_dy;

        if (nx < 0)
            nx = 0;

        if (ny < 0)
            ny = 0;

        if (nx > SCREEN_W - w->w)
            nx = SCREEN_W - w->w;

        if (ny > SCREEN_H - w->h)
            ny = SCREEN_H - w->h;

        if (nx == w->x && ny == w->y)
            return false;

        w->x = nx;
        w->y = ny;
        return true;
    }

    return false;
}