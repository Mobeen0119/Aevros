#ifndef KEYBOARD_H
#define KEYBOARD_H

extern unsigned char keyb[128];

void keyboard_handler();
char keyboard_getchar();

#endif