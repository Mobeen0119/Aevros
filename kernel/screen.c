#include "../Include/screen.h"
#include "../Include/terminal.h"
#define ROW 25
#define COL 80
#define VGA_COLOR 0x04

extern struct Line buffer[MAX_LINES];

extern int scroll_top;
extern void render(void);

int cursor_x = 0, cursor_y = 0;

static int out_col = 0;

void kclear_screen()
{
    buffer_init();

    cursor_y = 0;
    scroll_top = 0;
    out_col = 0;

    render();
}

// void kprint_at(const char *str, int row, int col)
// {
//     if (row >= ROW || col >= COL)
//         return;

//     int ind = (row * COL) + col;

//     for (int i = 0; str[i] != '\0'; i++)
//     {
//         if (col + i >= COL)
//             break;

//         buffer[ind + i] = (unsigned short)VGA_COLOR << 8 | str[i];
//     }
// }

void kput_char(char c)
{

    if (c == '\n')
    {
        out_col = 0;
        cursor_y++;
        if (cursor_y >= MAX_LINES)
            cursor_y = cursor_y % MAX_LINES;

        while (cursor_y >= scroll_top + HEIGHT)
            scroll_top++;
        return;
    }

    if (out_col >= WIDTH)
    {
        out_col = 0;
        cursor_y++;
        if (cursor_y >= MAX_LINES)
            cursor_y = cursor_y % MAX_LINES;

        while (cursor_y >= scroll_top + HEIGHT)
            scroll_top++;
    }

    buffer[cursor_y].text[out_col] = c;
    buffer[cursor_y].colors[out_col] = current_color;
    out_col++;
}

void kprint(const char *str)
{
    while (*str)
    {
        kput_char(*str++);
    }
    render();
}