#include "keyboard.h"
#include "../kernel/io.c"
#define KEY_RELEASE 0x80

char keyboard_buffer[128];
int keyb_head = 0, keyb_tail = 0;

void keyboard_push(char c)
{
    keyboard_buffer[keyb_head] = c;
    keyb_head = (keyb_head + 1) % 128;
}

char keyboard_getchar()
{
    while (keyb_head == keyb_tail)
        ;

    char c = keyboard_buffer[keyb_tail];
    keyb_tail = (keyb_tail + 1) % 128;

    return c;
}

void keyboard_handler()
{
    const char scan_code = inb(0x60); // Read the scan code from the keyboard data port

    if (scan_code & KEY_RELEASE)
        return;

    char letter = keyb[scan_code];
    if (letter == 0)
        return;

    keyboard_push(letter);
}
