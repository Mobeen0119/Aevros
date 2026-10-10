#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

void kprintf(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);

    vprintf(format, ap);
    va_end(ap);
}

void set_color(uint8_t fg, uint8_t bg)
{
    (void)bg;
    printf("\033[3%dm", fg & 7);
}

void reset_color(void)
{
    printf("\033[0m");
}