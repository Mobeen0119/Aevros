#ifndef KEYBOARD_H
#define KEYBOARD_H

extern volatile int keyb_head;
extern volatile int keyb_tail;

extern unsigned char keyb[128];

void keyboard_handler();
char keyboard_getchar();

#endif