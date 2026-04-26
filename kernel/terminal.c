#define WIDTH 80
#define HEIGHT 25
#define MAX_LINES 200

uint8_t current_color=0x0F;

char buffer[MAX_LINES][WIDTH];
uint8_t colors[MAX_LINES][WIDTH];

uint16_t *const vga_memory=(uint16_t*)0xB8000;

int cursor_x = 0;   // Columns
int cursor_y = 0;   // Logical line
int scroll_top = 0; // first line

// void clear_line(int line)
// {
//     int start = line * WIDTH;
//     for (int i = 0; i < WIDTH; i++)
//     {
//         vga_memory[start + i] = ' ' | (current_color << 8);
//     }
// }

void terminal_writeData(char c)
{
    if (c == '\n')
    {
        cursor_x = 0;
        cursor_y++;
        return;
    }
    buffer[cursor_y][cursor_x]=c;
    colors[cursor_y][cursor_x]=current_color;
    

    if (cursor_x >= WIDTH)
    {
        cursor_x = 0;
        cursor_y++;
    }

    if (cursor_y >= scroll_top + HEIGHT)   scroll_top++;


}

void render(){
    for (int y=0;y<HEIGHT;y++){
        int line = scroll_top+y;

        for (int x = 0; x < WIDTH; x++)
        {
            char c=buffer[line][x];
            uint8_t color=colors[line][x];

            vga_memory[y*WIDTH+x]=(uint16_t)c | (color << 8 );
        }
        
    }
}