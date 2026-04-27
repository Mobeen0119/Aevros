#include <stdint.h>
#include "../include/terminal.h"


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

        while (cursor_y >= scroll_top + HEIGHT)
            scroll_top++;
    }
}

void render()
{

    for (int y = 0; y < HEIGHT; y++) // History
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

    // Live Writing
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
            if (input_length < WIDTH)
            {
                vga_memory[input_row * WIDTH + cursor_x] =
                    ('_' | (current_color << 8));
            }
        }
    }
}