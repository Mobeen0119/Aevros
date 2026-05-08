#include <stdint.h>
#include <stdarg.h>
#include "../include/screen.h"

static void print_decimal(int num)
{

    char buffer[32];
    int i = 0, negative = 0;
    if (num == 0)
    {
        kput_char('0');
        return;
    }
    if (num < 0)
    {
        num = -num;
        negative = 1;
    }
    while (num > 0)
    {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }
    if (negative)
        buffer[i++] = '-';

    while (i--)
    {
        kput_char(buffer[i]);
    }
}

static void print_hex(uint32_t num)
{
    char *hex = "0123456789ABCDEF";
    for (int i = 28; i >= 0; i -= 4)
    {
        uint8_t digit = (num >> i) & 0xF;
        kput_char(hex[digit]);
    }
}
void kprintf(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    for (int i = 0; format[i] != '\0'; i++)
    {
        if (format[i] != '%')
        {
            kput_char(format[i]);
            continue;
        }
        i++;

        switch (format[i])
        {
        case 'c':
            char c = (char)va_arg(args, int);
            kput_char(c);
            break;

        case 's':
            char *s = va_arg(args, char *);
            if (!s)
                kprint("NULL");
            kprint(s);
            break;

        case 'd':
            int num = va_arg(args, int);
            print_decimal(num);
            break;

        case 'x':
            uint32_t num = va_arg(args, uint32_t);
            print_hex(num);
            break;

        case '%':
            kput_char('%');
            break;

        default:
            kput_char('%');
            kput_char(format[i]);
            break;
        }
    }
    va_end(args);
}
