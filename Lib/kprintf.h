#ifndef KPRINTF_H
#define KPRINTF_H

#include <stdint.h>
#include <stdarg.h>
#include "../include/screen.h"

void print_hex(uint32_t num);

void kprintf(const char *format, ...);

#endif // KPRINTF_H
