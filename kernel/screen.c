#include "../include/screen.h"
#define ROW 25
#define COL 80
#define VGA_COLOR 0x04

unsigned short *vga_buffer = (unsigned short *)0xB8000;

int cursor_x = 0, cursor_y = 0;

void kclear_screen()
{
    for (int i = 0; i < ROW * COL; i++)
    {
        vga_buffer[i] =
            (' ' | (VGA_COLOR << 8));
    }
    cursor_x = 0;
    cursor_y = 0;
}

void kprint_at(const char *str, int row, int col)
{
    if (row >= ROW || col >= COL)
        return;

    int ind = (row * COL) + col;

    for (int i = 0; str[i] != '\0'; i++)
    {
        vga_buffer[ind + i] = (unsigned short)VGA_COLOR << 8 | str[i];
    }
}

void kput_char(char c)
{

    if (c == '\n')
    {
        cursor_y = 0;
        cursor_x++;
        return;
    }

    if (cursor_y >= ROW)
    {
        cursor_y = 0;
        cursor_x++;
    }
    if (cursor_x >= COL)
    {
        cursor_x = 0;
    }

    int ind = (cursor_x * ROW) + COL;
    vga_buffer[ind] = ((unsigned short)VGA_COLOR << 8) | c;

    cursor_y++;
}

void kprint(const char *str)
{
    while (*str)
    {
        kput_char(*str++);
    }
}