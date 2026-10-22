#ifndef KPRINTF_H
#define KPRINTF_H

#include <stdint.h>
#include <stdarg.h>
#include "../Include/screen.h"

void print_hex(uint32_t num);

void kprintf(const char *format, ...);
void kprintf_set_net_chatty(int on);
int kprintf_net_chatty(void);

void ksnprintf(char *buff, int bufsz, const char *fmt, ...);

#endif
