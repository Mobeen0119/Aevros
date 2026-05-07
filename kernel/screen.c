#include "../include/screen.h"

unsigned short *vga_buffer = (unsigned short *)0xB8000;

const int Row = 25;
const int Col = 80;

void kclear_screen()
{
    for (int i = 0; i < Row * Col; i++)
    {
        vga_buffer[i] = (unsigned short)' ' | 0x0400;
    }
}

void kprint_at(const char *str, int row, int col)
{
    int ind = (Row * Col) + col;

    for (int i = 0; str[i] != '\0'; i++)
    {
        vga_buffer[ind + i] = (unsigned short)str[i] | 0x0400;
    }
}