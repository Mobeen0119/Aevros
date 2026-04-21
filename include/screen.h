unsigned short *vga_buffer = (unsigned short *)0xB8000;

const int Row = 25;
const int Col = 80; 

void clear_screen()
{
    for (int i = 0; i < Row * Col; i++)
    {
        vga_buffer[i] = (unsigned short)' ' | 0x7000;
    }
}

void print(const char *str, int row, int col)
{
    int ind = (Row * Col) + col;  // move pixel wise

    for (int i = 0; str[i] != '\0'; i++)
    {
        vga_buffer[ind + i] = (unsigned short)str[i] | 0x0400;
    }
}