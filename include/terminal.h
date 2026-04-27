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

uint16_t *const vga_memory = (uint16_t *)0xB8000;

char input_line[WIDTH];
int input_length = 0; // Columns
int cursor_y = 0;     // Logical line
int scroll_top = 0;   // first line
int cursor_x = 0;     // Input position in the current line
