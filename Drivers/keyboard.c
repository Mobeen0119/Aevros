#include "keyboard.h"
#include "../kernel/io.h"
#include "../Lib/kprintf.h"
#define KEY_RELEASE 0x80

unsigned char keyb[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0};

volatile int keyb_head = 0, keyb_tail = 0;
volatile char keyboard_buffer[128];

void keyboard_push(char c)
{
    keyboard_buffer[keyb_head] = c;
    keyb_head = (keyb_head + 1) % 128;
}

volatile char keyboard_getchar()
{
    while (keyb_head == keyb_tail)
        asm volatile("hlt");

    char c = keyboard_buffer[keyb_tail];
    keyb_tail = (keyb_tail + 1) % 128;
    return c;
}
void keyboard_handler()
{
    const char scan_code = inb(0x60);
    kprintf("SC=%x ", scan_code);

    if (scan_code & KEY_RELEASE)
    {
        outb(0x20, 0x20);
        return;
    }

    char letter = keyb[scan_code];
    if (letter != 0)
    {
        keyboard_push(letter);
        kprintf("PUSH=%c head=%d tail=%d\n", letter, keyb_head, keyb_tail);
    }

    outb(0x20, 0x20);
}
