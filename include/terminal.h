#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdint.h>
#define WIDTH 80
#define HEIGHT 25
#define MAX_LINES 8192

enum
{
    VGA_BLACK         = 0x0,
    VGA_BLUE          = 0x1,
    VGA_GREEN         = 0x2,
    VGA_CYAN          = 0x3,
    VGA_RED           = 0x4,
    VGA_MAGENTA       = 0x5,
    VGA_BROWN         = 0x6,
    VGA_LIGHT_GREY    = 0x7,
    VGA_DARK_GREY     = 0x8,
    VGA_LIGHT_BLUE    = 0x9,
    VGA_LIGHT_GREEN   = 0xA,
    VGA_LIGHT_CYAN    = 0xB,
    VGA_LIGHT_RED     = 0xC,
    VGA_LIGHT_MAGENTA = 0xD,
    VGA_YELLOW        = 0xE,
    VGA_WHITE         = 0xF
};

#define VGA_COLOR(fg,bg) (((bg)<<4)|(fg))

void set_color(uint8_t fg,uint8_t bg);
void reset_color(void);
void print_heading(const char *title);


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


void buffer_init();


void render();
void render_input_line(void);
void terminal_readline(char* out);

#endif
