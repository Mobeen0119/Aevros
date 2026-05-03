#include "keyboard.h"
#include "../kernel/io.c"

void keyboard_handler()
{
    const char scan_code = inb(0x60);   // Read the scan code from the keyboard data port

    if (scan_code & 0x80){}
        else
        {
            char letter = keyb[scan_code];
            print_char(letter);
        }
}
