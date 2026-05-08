#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>
#include <stdarg.h>

void kput_char(char c);
void kclear_screen();
void kprint_at(const char *str, int row, int col);\
void kprint(const char* str);

#endif