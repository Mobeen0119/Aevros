#include <stdint.h>
#include <stdarg.h>
#include "../include/screen.h"

static void print_decimal(int num);

void print_hex(uint32_t num);

void kprintf(const char *format, ...);
