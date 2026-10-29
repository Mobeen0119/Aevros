#ifndef GUI_TASK_H
#define GUI_TASK_H

#include <stdint.h>

#define GUI_KEY_UP 0x80
#define GUI_KEY_DOWN 0x81

void gui_push_key(uint8_t key);
void gui_start(void);

#endif