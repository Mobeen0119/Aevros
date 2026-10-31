#include "gui_task.h"
#include "net_view.h"
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

static void draw_cursor(void)
{
    aev_cross(nv_cursor_x - 4, nv_cursor_y - 4, 8, AEV_DAWN);
    aev_fill(nv_cursor_x, nv_cursor_y, 2, 2, AEV_TEXT);
}

static void draw_desktop(void)
{
    desktop_paint();
    wm_paint_all();
    draw_cursor();
    fb_flush();
    last_desktop_draw = get_ticks();
}

static void enter_mode(gui_mode_t next)
{
    if (mode == MODE_NETVIEW)
        network_view_hide();

    mode = next;

    if (next == MODE_SHELL)
    {
        fb_clear(0, 0, 0);
        terminal_show();
    }
    else if (next == MODE_NETVIEW)
    {
        network_view_open();
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
            network_view_mouse(nv_cursor_x, nv_cursor_y, 1);
        if (moved || pressed)
        {
            network_view_draw();
            draw_cursor();
            fb_flush();
        }
    }
    else if (mode == MODE_DESKTOP)
    {
        bool changed = wm_mouse(nv_cursor_x, nv_cursor_y, left_was_down, pressed, released);
        if (changed || moved)
            draw_desktop();
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

        if (mode == MODE_NETVIEW)
            netview_tick(get_ticks());

        else if (mode == MODE_DESKTOP && get_ticks() - last_desktop_draw >= DESKTOP_REFRESH_TICKS)
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