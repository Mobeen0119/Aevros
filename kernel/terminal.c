#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define TOTAL_LINES 200

uint16_t *const vga_memory = (uint16_t *)0xB8000;

int cursor_x = 0;   // Columns
int cursor_y = 0;   // Logical line
int scroll_top = 0; // first line

void clear_line(int line)
{
    int start = line * VGA_WIDTH;
    for (int i = 0; i < VGA_WIDTH; i++)
    {
        vga_memory[start + i] = ' ' | (current_color << 8);
    }
}

void terminal_writeData(char c)
{
    if (c == '\n')
    {
        cursor_x = 0;
        cursor_y++;
    }
    int physical_line = cursor_y % TOTAL_LINES;
    int physical_index = VGA_WIDTH * physical_line + cursor_x; // line->Flat Memory

    vga_memory[physical_index] = (uint16_t)c | (current_color << 8);
    cursor_x++;

    if (cursor_x >= VGA_WIDTH)
    {
        cursor_x = 0;
        cursor_y++;
    }
    clear_line(cursor_y % TOTAL_LINES);
}
