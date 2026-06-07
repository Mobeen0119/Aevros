#include <stdint.h>
#define WIDTH 80
#define HEIGHT 25
#define MAX_LINES 200

uint8_t current_color = 0x0F;

struct Line
{
    char text[80];
    uint8_t colors[80];
} buffer[MAX_LINES];

extern int cursor_x;
extern int cursor_y;
extern uint8_t current_color;

extern char input_line[];
extern int input_length;

extern int scroll_top;
extern void *buffer;
extern void *vga_memory;



void terminal_readline(char* out);