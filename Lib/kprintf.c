#include <stdint.h>
#include <stdarg.h>
#include "../Include/screen.h"

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

        int left_align = 0;

        if (format[i] == '-')
        {
            left_align = 1;
            i++;
        }
        int width = 0;

        while (format[i] >= '0' && format[i] <= '9')
        {
            width = width * 10 + (format[i] - '0');
            i++;
        }

        char temp[64];
        int len = 0;

        switch (format[i])
        {
        case 'c':
        {
            temp[0] = (char)va_arg(args, int);
            len = 1;
            break;
        }
        case 's':
        {
            char *s = va_arg(args, char *);
            if (!s)
                s = "NULL";
            while (s[len] && len < (int)sizeof(temp) - 1)
            {
                temp[len] = s[len];
                len++;
            }
            break;
        }
        case 'd':
        {
            int num = va_arg(args, int);
            int neg = 0;
            if (num < 0)
            {
                neg = 1;
                num = -num;
            }

            char rev[32];
            int r = 0;
            if (num == 0)
                rev[r++] = '0';
            while (num > 0)
            {
                rev[r++] = '0' + (num % 10);
                num /= 10;
            }
            if (neg)
                temp[len++] = '-';
            while (r--)
                temp[len++] = rev[r];
            break;
        }
        case 'u':
        {
            uint32_t num = va_arg(args, uint32_t);
            char rev[32];
            int r = 0;
            if (num == 0)
                rev[r++] = '0';

            while (num > 0)
            {
                rev[r++] = '0' + (num % 10);
                num /= 10;
            }

            while (r--)
                temp[len++] = rev[r];
            break;
        }
        case 'x':
        {
            uint32_t num = va_arg(args, uint32_t);
            const char *hex = "0123456789ABCDEF";
            char rev[32];

            int r = 0;
            if (num == 0)
                rev[r++] = '0';
            while (num > 0)
            {
                rev[r++] = hex[num & 0xF];
                num >>= 4;
            }

            while (r--)
                temp[len++] = rev[r];
            break;
        }

        case '%':
            temp[len++] = '%';
            break;

        default:
            temp[len++] = '%';
            if (format[i])
                temp[len++] = format[i];
            break;
        }

        int pad = width - len;
      
        if (pad > 0 && !left_align)
        {
            for (int p = 0; p < pad; p++)
                kput_char(' ');
        }

        for (int k = 0; k < len; k++)
            kput_char(temp[k]);

        if (pad > 0 && left_align)
            for (int p = 0; p < pad; p++)
                kput_char(' ');
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
            buff[pos++] = fmt[i];
            continue;
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
