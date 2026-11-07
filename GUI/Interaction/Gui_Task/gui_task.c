#include "gui_task.h"
#include "../NetView/net_view.h"
#include "../../WM/wm.h"
#include "../../WM/health_window.h"
#include "../../WM/desktop.h"
#include "../../WM/style.h"
#include "../../Framebuffer/framebuffer.h"
#include "../../Algo/algo.h"
#include "../../../Drivers/Mouse/mouse.h"
#include "../../../Include/terminal.h"
#include "../../../kernel/Process/task.h"

#include <stdbool.h>
#include "../../../Lib/string.h"

#define KEY_QUEUE_LEN 16
#define GUI_SCREEN_W 1024
#define GUI_SCREEN_H 768
#define DESKTOP_REFRESH_TICKS 20

void keyboard_push(char c);

typedef enum
{
    MODE_SHELL,
    MODE_NETVIEW,
    MODE_DESKTOP
} gui_mode_t;

static volatile uint8_t key_queue[KEY_QUEUE_LEN];
static volatile int key_head;
static volatile int key_tail;

static volatile gui_mode_t mode = MODE_NETVIEW;
static int32_t nv_cursor_x = GUI_SCREEN_W / 2;
static int32_t nv_cursor_y = GUI_SCREEN_H / 2;
static bool left_was_down;
static uint32_t last_desktop_draw;
static uint32_t desktop_seen_terminal;
static health_sig_t desktop_seen_health;

void gui_push_key(uint8_t key)
{
    int next = (key_head + 1) % KEY_QUEUE_LEN;
    if (next == key_tail)
        return;

    key_queue[key_head] = key;
    key_head = next;
}

int gui_active(void)
{
    return mode != MODE_SHELL;
}

#define CURSOR_HALF 5
#define CURSOR_SPAN (2 * CURSOR_HALF + 1)

static uint32_t cursor_under[CURSOR_SPAN * CURSOR_SPAN];
static int32_t cursor_shown_x;
static int32_t cursor_shown_y;
static bool cursor_shown;

static void cursor_restore(void)
{
    for (int dy = -CURSOR_HALF; dy <= CURSOR_HALF; dy++)
        for (int dx = -CURSOR_HALF; dx <= CURSOR_HALF; dx++)
            fb_put_pixel(cursor_shown_x + dx, cursor_shown_y + dy,
                         cursor_under[(dy + CURSOR_HALF) * CURSOR_SPAN + (dx + CURSOR_HALF)]);
}

static void cursor_paint(void)
{
    for (int dy = -CURSOR_HALF; dy <= CURSOR_HALF; dy++)
        for (int dx = -CURSOR_HALF; dx <= CURSOR_HALF; dx++)
            cursor_under[(dy + CURSOR_HALF) * CURSOR_SPAN + (dx + CURSOR_HALF)] =
                fb_get_pixel(nv_cursor_x + dx, nv_cursor_y + dy);

    aev_cross(nv_cursor_x - 4, nv_cursor_y - 4, 8, AEV_TEXT);
    aev_fill(nv_cursor_x, nv_cursor_y, 2, 2, AEV_BG);

    cursor_shown_x = nv_cursor_x;
    cursor_shown_y = nv_cursor_y;
    cursor_shown = true;
}

static void cursor_move_flush(void)
{
    if (!cursor_shown)
    {
        cursor_paint();
        fb_flush_rect(nv_cursor_x - CURSOR_HALF, nv_cursor_y - CURSOR_HALF, CURSOR_SPAN, CURSOR_SPAN);
        return;
    }

    int32_t old_x = cursor_shown_x;
    int32_t old_y = cursor_shown_y;

    cursor_restore();
    cursor_paint();

    int32_t x0 = (old_x < nv_cursor_x ? old_x : nv_cursor_x) - CURSOR_HALF;
    int32_t y0 = (old_y < nv_cursor_y ? old_y : nv_cursor_y) - CURSOR_HALF;
    int32_t x1 = (old_x > nv_cursor_x ? old_x : nv_cursor_x) + CURSOR_HALF;
    int32_t y1 = (old_y > nv_cursor_y ? old_y : nv_cursor_y) + CURSOR_HALF;
    fb_flush_rect(x0, y0, x1 - x0 + 1, y1 - y0 + 1);
}

static void draw_desktop(void)
{
    desktop_paint();
    wm_paint_all();
    cursor_paint();
    fb_flush();
    last_desktop_draw = get_ticks();
    desktop_seen_terminal = terminal_generation();
    desktop_seen_health = health_window_signature();
}

static bool desktop_content_changed(void)
{
    health_sig_t now = health_window_signature();
    return terminal_generation() != desktop_seen_terminal ||
           memcmp(&now, &desktop_seen_health, sizeof(health_sig_t)) != 0;
}

static void enter_mode(gui_mode_t next)
{
    if (mode == MODE_NETVIEW)
        network_view_hide();

    mode = next;
    cursor_shown = false;

    if (next == MODE_SHELL)
    {
        fb_clear(0, 0, 0);
        terminal_show();
    }
    else if (next == MODE_NETVIEW)
    {
        network_view_open();
        cursor_paint();
        fb_flush();
    }
    else
    {
        draw_desktop();
    }
}

static void handle_key(uint8_t key)
{
    if (key == 27)
    {
        enter_mode(mode == MODE_SHELL ? MODE_NETVIEW : MODE_SHELL);
        return;
    }

    if (key == GUI_KEY_F2)
    {
        enter_mode(mode == MODE_DESKTOP ? MODE_SHELL : MODE_DESKTOP);
        return;
    }

    if (mode == MODE_DESKTOP)
    {
        if (key < 0x80)
            keyboard_push((char)key);
        return;
    }

    if (mode != MODE_NETVIEW)
        return;

    if (key == GUI_KEY_UP)
        network_view_move_selection(-1);
    else if (key == GUI_KEY_DOWN)
        network_view_move_selection(1);
    else if (key == 'r' || key == 'R')
        network_view_clear_selection();
}

static void handle_mouse(void)
{
    mouse_event_t ev;
    bool moved = false;
    bool pressed = false;
    bool released = false;

    while (ps2_mouse_poll_event(&ev))
    {
        nv_cursor_x += ev.dx;
        nv_cursor_y -= ev.dy;

        if (nv_cursor_x < 0)
            nv_cursor_x = 0;

        if (nv_cursor_y < 0)
            nv_cursor_y = 0;

        if (nv_cursor_x > GUI_SCREEN_W - 1)
            nv_cursor_x = GUI_SCREEN_W - 1;

        if (nv_cursor_y > GUI_SCREEN_H - 1)
            nv_cursor_y = GUI_SCREEN_H - 1;

        if (ev.dx || ev.dy)
            moved = true;

        if (ev.left_button && !left_was_down)
            pressed = true;

        if (!ev.left_button && left_was_down)
            released = true;

        left_was_down = ev.left_button;
    }

    if (mode == MODE_NETVIEW)
    {
        if (pressed)
        {
            network_view_mouse(nv_cursor_x, nv_cursor_y, 1);
            network_view_draw();
            cursor_paint();
            fb_flush();
        }
        else if (moved)
        {
            cursor_move_flush();
        }
    }
    else if (mode == MODE_DESKTOP)
    {
        bool changed = wm_mouse(nv_cursor_x, nv_cursor_y, left_was_down, pressed, released);
        if (changed)
            draw_desktop();
        else if (moved)
            cursor_move_flush();
    }
}

static void gui_task(void)
{
    for (;;)
    {
        while (key_tail != key_head)
        {
            uint8_t key = key_queue[key_tail];
            key_tail = (key_tail + 1) % KEY_QUEUE_LEN;
            handle_key(key);
        }

        handle_mouse();

        if (mode == MODE_NETVIEW && netview_tick(get_ticks()))
        {
            cursor_paint();
            fb_flush();
        }

        else if (mode == MODE_DESKTOP && get_ticks() - last_desktop_draw >= DESKTOP_REFRESH_TICKS &&
                 desktop_content_changed())
            draw_desktop();

        asm volatile("hlt");
    }
}

void gui_start(void)
{
    wm_init();
    wm_open("TERMINAL", 24, 44, 962, 500, terminal_paint, terminal_status, terminal_why);

    wm_open("SYSTEM HEALTH", 440, 260, 560, 230, health_window_paint, health_window_status, health_window_why);
    task_create_kernel(gui_task);
}