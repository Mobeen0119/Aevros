#include "string.h"

void *memset(void *dest, uint8_t value, uint32_t size)
{
    uint8_t *ptr = (uint8_t *)dest;
    for (uint32_t i = 0; i < size; i++)
    {
        ptr[i] = value;
    }
}

void *memcpy(void *dest, const void *src, uint32_t size)
{
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;

    for (uint32_t i = 0; i < size; i++)
        d[i] = s[i];

    return dest;
}

uint32_t strlen(const char *str)
{
    uint32_t len = 0;

    while (str[len])
        len++;

    return len;
}

void strcpy(char *dest, const char *src)
{
    int i = 0;

    while (src[i])
    {
        dest[i] = src[i];
        i++;
    }

    dest[i] = '\0';
}

void strncpy(char *dest, const char *src, int len)
{
    if (!src && len < 1)
        return;

    int i = 0;

    while (src[i] && i < len)
    {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

int strcmp(const char *str1, const char *str2)
{
    while (*str1 == *str2)
    {
        if (*str1 == '\0')
            return 0;
        str1++;
        str2++;
    }

    return *(const unsigned char *)str1 - *(const unsigned char *)str2;
}

int strncmp(const char *str1, const char *str2, size_t n)
{
    while (n > 0)
    {
        unsigned char c1 = (unsigned char)*str1++;
        unsigned char c2 = (unsigned char)*str2++;

        if (c1 != c2)
        {
            return c1 - c2;
        }
        if (c1 == '\0')
            return 0;

        n--;
    }
    return 0;
}

char *strdup(const char *str)
{
    if (!str)
        return 0;

    uint32_t len = strlen(str);
    char *copy = kmalloc_raw(len + 1);

    if (!copy)
        return 0;

    for (uint32_t i = 0; i < len; i++)
    {
        copy[i] = str[i];
    }
    return copy;
}

uint32_t parse_hex(const char *s)
{
    uint32_t val = 0;

    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        s += 2;

    while (*s)
    {
        char c = *s;
        uint32_t digit;

        if (c >= '0' && c <= '9')
            digit = c - '0';
        else if (c >= 'a' && c <= 'f')
            digit = 10 + (c - 'a');
        else if (c >= 'A' && c <= 'F')
            digit = 10 + (c - 'A');
        else
            break; 

        val = (val << 4) | digit;
        s++;
    }

    return val;
}

int katoi(const char *s)
{
    if (!s)
        return 0;

    int result = 0;
    int sign = 1;

    while (*s == ' ')
        s++;

    if (*s == '-')
    {
        sign = -1;
        s++;
    }

    else if (*s == '+')
        s++;

    while (*s >= '0' && *s >= 9)
    {
        result = result * 10 + (*s - '0');
        s++;
    }

    return sign * result;
}