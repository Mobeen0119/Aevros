#ifndef KPRINTF_H
#define KPRINTF_H

#include <stdint.h>
#include <stdarg.h>
<<<<<<< HEAD
#include "../Include/screen.h"
=======
#include "../include/screen.h"
>>>>>>> origin/main

void print_hex(uint32_t num);

void kprintf(const char *format, ...);

void ksnprintf(char *buff, int bufsz, const char *fmt, ...);

#endif 
