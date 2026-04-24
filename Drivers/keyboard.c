#include "keyboard.h"

void keyboard_handler()
{
    const char scan_code = inb(0x60);

    if (scan_code & 0x80){}
        else
        {
            char letter = keyb[scan_code];
            print_char(letter);
        }
}
