#include <stdint.h>
#include "../kernel/Memory/kheap.h"

uint32_t strlen(const char *str);

void strcpy(char *dest, const char *src);

int strcmp(const char *str1, const char *str2);

int strncmp(const char *str1, const char *str2, size_t n);

char *strdup(const char *str);

int katoi(const char *s);