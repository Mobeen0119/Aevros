#include <stdint.h>
#include "../kernel/Memory/kheap.c"

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

char *strdup(const char *str)
{
    if (!str)
        return 0;

    uint32_t len = strlen(str);
    char *copy = kmalloc(len + 1);

    if (!copy)
        return 0;

    for (uint32_t i = 0; i < len; i++)
    {
        copy[i] = str[i];
    }
    return copy;
}