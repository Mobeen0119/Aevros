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

void print_hex(uint32_t num)
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
            uint32_t unum = va_arg(args, uint32_t);
            print_hex(unum);
            break;

        case '%':
            kput_char('%');
            break;

        case 'u':
            uint32_t mnum = va_arg(args, uint32_t);
            print_decimal((int)mnum); 
            break;

        default:
            kput_char('%');
            kput_char(format[i]);
            break;
        }
    }
    va_end(args);
}

void ksnprintf(char *buff, int bufsz, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int pos = 0;

    for (int i = 0; fmt[i] && pos < bufsz - 1; i++)
    {
        if (fmt[i] != '%')
        {
            buff[pos++]=fmt[i]; continue;
        }

        i++;

        switch (fmt[i])
        {
        case 'u':
        {
            uint32_t n = va_arg(args, uint32_t);
            char temp[12];
            int t = 0;
            if (!n)
                temp[t++] = '0';

            while (n)
            {
                temp[t++] = '0' + (n % 10);
                n /= 10;
            }
           for (int j = t - 1; j >= 0 && pos < bufsz - 1; j--)
                buff[pos++] = temp[j];
            break;
        }
        case 'd':
        {
            int n = va_arg(args, int);
            if (n < 0 && pos < bufsz - 1)
            {
                buff[pos++] = '-';
                n = -n;
            }
            char tmp[12];
            int t = 0;
            if (!n)
                tmp[t++] = '0';
            while (n)
            {
                tmp[t++] = '0' + (n % 10);
                n /= 10;
            }
         for (int j = t - 1; j >= 0 && pos < bufsz - 1; j--)
             buff[pos++] = tmp[j];
         break;
        }
        case 's':
        {
            char *s = va_arg(args, char *);
            if (!s)
                s = "(null)";
            while (*s && pos < bufsz - 1)
                buff[pos++] = *s++;
            break;
        }
        case 'x':
        {
            uint32_t n = va_arg(args, uint32_t);
            char *hex = "0123456789abcdef";
            char tmp[8];
            int t = 0;
            if (!n)
                tmp[t++] = '0';
            while (n)
            {
                tmp[t++] = hex[n & 0xF];
                n >>= 4;
            }
           for (int j = t - 1; j >= 0 && pos < bufsz - 1; j--)
              buff[pos++] = tmp[j];
            break;
        }
        case '%':
            buff[pos++] = '%';
            break;
        }
    }
    buff[pos] = '\0';
    va_end(args);
}
