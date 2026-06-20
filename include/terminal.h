#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdint.h>
#define WIDTH 80
#define HEIGHT 25
#define MAX_LINES 200

struct Line
{
    char text[80];
    uint8_t colors[80];
};

extern int cursor_x;
extern int cursor_y;
extern uint8_t current_color;

extern char input_line[];
extern int input_length;

extern int scroll_top;
extern struct Line buffer[MAX_LINES];
extern uint16_t *const vga_memory;



void terminal_readline(char* out);
void render();

#endif
