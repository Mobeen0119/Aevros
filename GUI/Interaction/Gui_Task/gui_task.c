#include "gui_task.h"
#include "../NetView/net_view.h"
#include "../../Framebuffer/framebuffer.h"
#include "../../Algo/algo.h"
#include "../../../Drivers/Mouse/mouse.h"
#include "../../../Include/terminal.h"
#include "../../../kernel/Process/task.h"
#include <stdbool.h>

#define KEY_QUEUE_LEN 16
#define GUI_SCREEN_W 1024
#define GUI_SCREEN_H 768

static volatile uint8_t key_queue[KEY_QUEUE_LEN];
static volatile int key_head;
static volatile int key_tail;

static int32_t nv_cursor_x = GUI_SCREEN_W / 2;
static int32_t nv_cursor_y = GUI_SCREEN_H / 2;
static bool left_was_down;

void gui_push_key(uint8_t key)
{
    int next = (key_head + 1) % KEY_QUEUE_LEN;
    if (next == key_tail)
        return;

    key_queue[key_head] = key;
    key_head = next;
}

static void toggle_netview(void)
{
    if (network_view_is_visible())
    {
        network_view_hide();
        fb_clear(0, 0, 0);
        terminal_show();
    }
    else
    {
        network_view_open();
    }
}

static void handle_key(uint8_t key)
{
    if (key == 27)
    {
        toggle_netview();
        return;
    }

    if (!network_view_is_visible())
        return;

    if (key == GUI_KEY_UP)
        network_view_move_selection(-1);

    else if (key == GUI_KEY_DOWN)
        network_view_move_selection(1);

    else if (key == 'r' || key == 'R')
        network_view_clear_selection();
}

static void draw_cursor(void)
{
    fb_rect_filled(nv_cursor_x, nv_cursor_y, 6, 6, 255, 255, 255);
}

static void handle_mouse(void)
{
    mouse_event_t ev;
    bool moved = false;
    bool clicked = false;

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
            clicked = true;
        left_was_down = ev.left_button;

        if (clicked && network_view_is_visible())
            network_view_mouse(nv_cursor_x, nv_cursor_y, 1);
    }

    if (network_view_is_visible() && (moved || clicked))
    {
        network_view_draw();
        draw_cursor();
        fb_flush();
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

        if (network_view_is_visible())
            netview_tick(get_ticks());

        asm volatile("hlt");
    }
}

void gui_start(void)
{
    task_create_kernel(gui_task);
}