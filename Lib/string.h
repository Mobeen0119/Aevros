#include <stdint.h>
#include "../kernel/Memory/kheap.h"

uint32_t strlen(const char *str);

void strcpy(char *dest, const char *src);

int strcmp(const char *str1, const char *str2);

int strncmp(const char *str1, const char *str2, size_t n);

char *strdup(const char *str);

void *memcpy(void *dest, const void *src, uint32_t size);

void *memset(void *dest, uint8_t value, uint32_t size);

int katoi(const char *s);

uint32_t parse_hex(const char *s);

void strncpy(char *dest, const char *src, int len);
