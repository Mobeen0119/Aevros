#include <stdint.h>
#include "../Include/terminal.h"
#include "../Drivers/keyboard.h"
#include "../Include/screen.h"
#include "../Lib/kprintf.h"
#include "../GUI/Framebuffer/framebuffer.h"
#include "../GUI/Algo/Font/font.h"
#include "../GUI/Algo/algo.h"
#include "../GUI/Interaction/NetView/net_view.h"
#include "../GUI/Interaction/Gui_Task/gui_task.h"
#include "../GUI/WM/style.h"

uint16_t *const vga_memory = (uint16_t *)0xB8000;

#define CELL_W 12
#define CELL_H 18
#define GLYPH_SCALE 2

static uint16_t shadow[WIDTH * HEIGHT];
static uint16_t cells[WIDTH * HEIGHT];

static uint32_t vga_rgb(uint8_t c)
{
    static const uint32_t pal[16] = {
        0x07090D, 0x3A5F8C, 0x5BD39A, 0x6FB7C9, 0xC2572A, 0x9A6FB0, 0x8A6A2E, 0x9AA3B0,
        0x5A6470, 0x8FB8E8, 0x8BE8B8, 0x9FE0EA, 0xE06060, 0xD9B77A, 0xE8A33D, 0xE6E9EF};

    return pal[c & 0xF];
}

static void draw_cell_at(int px, int py, uint16_t cell)
{

    uint32_t bg = vga_rgb((uint8_t)(cell >> 12));
    uint32_t fg = vga_rgb((uint8_t)(cell >> 8));

    fb_rect_filled(px, py, CELL_W, CELL_H, (bg >> 16) & 0xFF, (bg >> 8) & 0xFF, bg & 0xFF);
    font_draw_char_scaled(px + 1, py + 1, (char)(cell & 0xFF), fg, GLYPH_SCALE);
}

static void put_cell(int idx, uint16_t cell)
{
    if (idx < 0 || idx >= WIDTH * HEIGHT)
        return;

    cells[idx] = cell;

    if (!framebuffer_available())
    {
        vga_memory[idx] = cell;
        return;
    }

    if (gui_active())
        return;

    if (shadow[idx] == cell)
        return;

    shadow[idx] = cell;
    draw_cell_at((idx % WIDTH) * CELL_W, (idx / WIDTH) * CELL_H, cell);
}

void terminal_paint(int x, int y, int w, int h)
{
    (void)w;
    (void)h;

    for (int r = 0; r < HEIGHT; r++)
        for (int c = 0; c < WIDTH; c++)
            draw_cell_at(x + c * CELL_W, y + r * CELL_H, cells[r * WIDTH + c]);
}

const char *terminal_status(uint32_t *color)
{
    *color = 0x5BD39A;
    return "READY";
}

void terminal_why(char *buf, int cap)
{
    int len = 0;
    len = aev_put_str(buf, len, cap, "INPUT GOES TO THE SHELL  ESC FULL SCREEN  F2 LEAVES DESKTOP");
    (void)len;
}

void terminal_show(void)
{
    for (int i = 0; i < WIDTH * HEIGHT; i++)
        shadow[i] = 0xFFFF;
    render();
}

struct Line buffer[MAX_LINES];

char input_line[WIDTH];

int input_length = 0;
extern int cursor_y;

int scroll_top = 0;
extern int cursor_x;

uint8_t current_color = VGA_COLOR(VGA_LIGHT_GREY, VGA_BLACK);

void set_color(uint8_t fg, uint8_t bg)
{
    current_color = VGA_COLOR(fg, bg);
}

void reset_color(void)
{
    current_color = VGA_COLOR(VGA_LIGHT_GREY, VGA_BLACK);
}

void print_heading(const char *title)
{
    set_color(VGA_CYAN, VGA_BLACK);
    kprint("\n  == ");
    kprint(title);
    kprint(" ==\n");
    reset_color();
    set_color(VGA_WHITE, VGA_BLACK);
}

void buffer_init()
{
    for (int i = 0; i < MAX_LINES; i++)
    {
        for (int j = 0; j < WIDTH; j++)
        {
            buffer[i].text[j] = '\0';
            buffer[i].colors[j] = current_color;
        }
    }
}

void move_right()
{
    if (cursor_x < input_length)
        cursor_x++;
}

void move_left()
{
    if (cursor_x > 0)
        cursor_x--;
}

void insert_char(char c)
{
    if (input_length >= WIDTH - 1)
        return;

    for (int i = input_length; i > cursor_x; i--)
    {
        input_line[i] = input_line[i - 1];
    }
    input_line[cursor_x] = c;
    cursor_x++;
    input_length++;
}

void delete_char()
{
    if (cursor_x >= input_length)
        return;

    for (int i = cursor_x; i < input_length - 1; i++)
    {
        input_line[i] = input_line[i + 1];
    }

    input_length--;
}

void handle_char(char c)
{
    insert_char(c);
}

void handle_backspace()
{
    if (cursor_x == 0)
        return;

    for (int i = cursor_x - 1; i < input_length - 1; i++)
    {
        input_line[i] = input_line[i + 1];
    }
    cursor_x--;
    input_length--;
}

void handle_enter()
{
    if (input_length > 0)
    {
        for (int i = 0; i < input_length; i++)
        {
            buffer[cursor_y].text[i] = input_line[i];
            buffer[cursor_y].colors[i] = current_color;
        }
        buffer[cursor_y].text[input_length] = '\0';

        cursor_y++;
        if (cursor_y >= MAX_LINES)
        {
            cursor_y = cursor_y % MAX_LINES;
        }
        for (int i = 0; i < WIDTH; i++)
            input_line[i] = '\0';
        input_length = 0;

        if (scroll_top + HEIGHT >= cursor_y)
        {
            if (cursor_y >= HEIGHT)
                scroll_top = cursor_y - HEIGHT + 1;
            else
                scroll_top = 0;
        }
    }
}

static void render_frame(void)
{
    for (int y = 0; y < HEIGHT; y++)
    {
        int line = scroll_top + y;
        if (line >= MAX_LINES)
            continue;
        for (int x = 0; x < WIDTH; x++)
        {
            char c = buffer[line].text[x];
            uint8_t color = buffer[line].colors[x];
            put_cell(y * WIDTH + x, (uint16_t)c | (color << 8));
        }
    }

    render_input_line();
}

void render(void)
{
    render_frame();

    if (framebuffer_available() && !gui_active())
        fb_flush();
}

void render_input_line(void)
{
    int input_row = cursor_y - scroll_top;
    if (input_row >= 0 && input_row < HEIGHT)
    {
        for (int x = 0; x < input_length; x++)
        {

            put_cell(input_row * WIDTH + x, (uint16_t)input_line[x] | (current_color << 8));
        }
        for (int x = input_length; x < WIDTH; x++)
        {
            put_cell(input_row * WIDTH + x, (uint16_t)' ' | (current_color << 8));
        }
        if (input_length < WIDTH)
        {
            put_cell(input_row * WIDTH + cursor_x, ('_' | (current_color << 8)));
        }
    }
}

void terminal_readline(char *out)
{
    int i = 0;
    char c;

    input_length = 0;
    cursor_x = 10;

    while (1)
    {
        c = keyboard_getchar();

        if (c == '\n')
        {
            set_color(VGA_LIGHT_MAGENTA, VGA_BLACK);
            for (int k = 0; k < i; k++)
                kput_char(input_line[k]);
            reset_color();

            kput_char(c);
            break;
        }
        else if (c == '\b' && i > 0)
        {
            handle_backspace();
            i = input_length;
        }
        else
        {
            if (i < WIDTH - 1)
            {
                input_line[i++] = c;
                handle_char(c);
            }
        }
        render_input_line();
    }
    input_line[i] = '\0';

    for (int j = 0; j <= i; j++)
        out[j] = input_line[j];

    for (int j = 0; j < WIDTH; j++)
        input_line[j] = '\0';
    input_length = 0;
    cursor_x = 0;
}