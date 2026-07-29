#ifndef MENU_H
#define MENU_H

#define MENU_CAPACITY 32

#include <stdint.h>

int menu_open_port(uint16_t port, uint8_t protocol);
void menu_close_port(uint16_t port, uint8_t protocol);
int menu_is_open(uint16_t port, uint8_t protocol);
uint32_t menu_count(void);

#endif
