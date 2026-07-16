#include <stdint.h>
#include "../Include/terminal.h"
#include "../Drivers/keyboard.h"
#include "../Include/screen.h"
#include "../Lib/kprintf.h"

uint16_t *const vga_memory = (uint16_t *)0xB8000;

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

void render()
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
            vga_memory[y * WIDTH + x] =
                (uint16_t)c | (color << 8);
        }
    }

    render_input_line();
}

void render_input_line(void)
{
    int input_row = cursor_y - scroll_top;
    if (input_row >= 0 && input_row < HEIGHT)
    {
        for (int x = 0; x < input_length; x++)
        {

            vga_memory[input_row * WIDTH + x] = (uint16_t)input_line[x] | (current_color << 8);
        }
        for (int x = input_length; x < WIDTH; x++)
        {
            vga_memory[input_row * WIDTH + x] =
                (uint16_t)' ' | (current_color << 8);
        }
        if (input_length < WIDTH)
        {
            vga_memory[input_row * WIDTH + cursor_x] =
                ('_' | (current_color << 8));
        }
    }
}

void terminal_readline(char *out)
{
    int i = 0;
    char c;

    input_length = cursor_x = 0;

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