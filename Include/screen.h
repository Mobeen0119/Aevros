#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>
#include <stdarg.h>

void kput_char(char c);
void kclear_screen();

void kprint(const char* str);

#endif