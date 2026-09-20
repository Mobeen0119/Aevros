#ifndef FONT_H
#define FONT_H

#include <stdint.h>
#include <stdbool.h>

#define FONT_GLYPH_W 5
#define FONT_GLYPH_H 7

#define FONT_ADVANCE (FONT_GLYPH_W + 1) // 1px gap between characters

#define FONT_MISSING_LOG_LEN 16

void font_draw_char(int32_t x, int32_t y, char c, uint32_t color);
void font_draw_string(int32_t x, int32_t y, const char *str, uint32_t color);

uint32_t font_string_width(const char *str);

const char *font_dependency_note(void);

bool font_selftest(void);

typedef struct
{
    char requested;
    int32_t x, y;

    uint64_t tick;

} font_missing_entry_t;

uint32_t font_get_missing_log(font_missing_entry_t *out, uint32_t max_entries);

#endif