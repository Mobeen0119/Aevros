#include "font.h"
#include "../../../Lib/string.h"

extern void fb_put_pixel(int32_t x, int32_t y, uint32_t color);

extern uint64_t get_ticks(void);

#define GLYPH_COUNT 45

static const uint8_t glyphs[GLYPH_COUNT][FONT_GLYPH_H] = {

    // 0-9

    {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E},

    {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E},
    {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F},
    {0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E},
    {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02},

    {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E},
    {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E},

    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08},
    {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E},
    {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C},

    // Alphabets A-Z

    {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},

    {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E},
    {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E},
    {0x1C, 0x12, 0x11, 0x11, 0x11, 0x12, 0x1C},
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F},
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10},
    {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F},

    {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},
    {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E},
    {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C},
    {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11},
    {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F},
    {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11},
    {0x11, 0x19, 0x15, 0x15, 0x13, 0x11, 0x11},
    {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},
    {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10},
    {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D},
    {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11},

    {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E},
    {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04},
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},

    {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04},
    {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A},

    {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11},
    {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04},
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F},

    // space . : / % - _ ' ,
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C},
    {0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00},

    {0x01, 0x02, 0x04, 0x04, 0x08, 0x10, 0x10},

    {0x19, 0x1A, 0x02, 0x04, 0x08, 0x0B, 0x13},
    {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00},

    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F},
    {0x04, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x08},
};

static int glyph_index(char c)
{
    if (c >= 'a' && c <= 'z')
        c -= 32; // fold to uppercase glyph
    if (c >= '0' && c <= '9')
        return c - '0';

    if (c >= 'A' && c <= 'Z')
        return 10 + (c - 'A');
    switch (c)
    {

    case ' ':
        return 36;

    case '.':
        return 37;
    case ':':
        return 38;

    case '/':
        return 39;
    case '%':
        return 40;

    case '-':
        return 41;
    case '_':
        return 42;
    case '\'':
        return 43;
    case ',':
        return 44;
    default:
        return -1;
    }
}

static font_missing_entry_t missing_log[FONT_MISSING_LOG_LEN];
static uint32_t missing_head = 0;

static uint32_t missing_count = 0;

static void push_missing(char c, int32_t x, int32_t y)
{
    missing_log[missing_head] = (font_missing_entry_t){c, x, y, get_ticks()};

    missing_head = (missing_head + 1) % FONT_MISSING_LOG_LEN;

    if (missing_count < FONT_MISSING_LOG_LEN)
        missing_count++;
}

static void draw_missing_glyph(int32_t x, int32_t y, uint32_t color)
{
    for (int col = 0; col < FONT_GLYPH_W; col++)
    {
        fb_put_pixel(x + col, y, color);

        fb_put_pixel(x + col, y + FONT_GLYPH_H - 1, color);
    }
    for (int row = 0; row < FONT_GLYPH_H; row++)
    {
        fb_put_pixel(x, y + row, color);
        fb_put_pixel(x + FONT_GLYPH_W - 1, y + row, color);
    }
}

void font_draw_char(int32_t x, int32_t y, char c, uint32_t color)
{
    int idx = glyph_index(c);

    if (idx < 0)
    {
        draw_missing_glyph(x, y, color);
        push_missing(c, x, y);

        return;
    }
    for (int row = 0; row < FONT_GLYPH_H; row++)
    {
        uint8_t bits = glyphs[idx][row];

        for (int col = 0; col < FONT_GLYPH_W; col++)
        {
            if (bits & (1 << (FONT_GLYPH_W - 1 - col)))
            {
                fb_put_pixel(x + col, y + row, color);
            }
        }
    }
}

void font_draw_string(int32_t x, int32_t y, const char *str, uint32_t color)
{
    int32_t cursor_x = x;

    for (uint32_t i = 0; str[i]; i++)
    {
        font_draw_char(cursor_x, y, str[i], color);

        cursor_x += FONT_ADVANCE;
    }
}

uint32_t font_string_width(const char *str)
{
    uint32_t len = strlen(str);

    return len == 0 ? 0 : len * FONT_ADVANCE - 1; // no trailing gap after the last glyph
}

const char *font_dependency_note(void)
{
    return "Every other module can track state perfectly, but nothing becomes readable on screen ..... window titles, the launcher list, timestamps, all of it";
}

uint32_t font_get_missing_log(font_missing_entry_t *out, uint32_t max_entries)
{

    uint32_t n = missing_count < max_entries ? missing_count : max_entries;
    uint32_t oldest = (missing_head + FONT_MISSING_LOG_LEN - missing_count) % FONT_MISSING_LOG_LEN;

    for (uint32_t i = 0; i < n; i++)
        out[i] = missing_log[(oldest + i) % FONT_MISSING_LOG_LEN];
    return n;
}

bool font_selftest(void)
{

    if (glyph_index('a') != glyph_index('A'))
        return false; // lowercase to uppercase

    if (glyph_index('0') != 0)
        return false;

    if (glyph_index('9') != 9)
        return false;
    if (glyph_index('A') != 10)
        return false;

    if (glyph_index('Z') != 35)
        return false;
    if (glyph_index(' ') != 36)
        return false;

    if (glyph_index('?') != -1)
        return false; // unmapped

    if (font_string_width("") != 0)
        return false;
    if (font_string_width("A") != FONT_GLYPH_W)
        return false;

    if (font_string_width("AB") != FONT_GLYPH_W * 2 + 1)
        return false;

    font_missing_entry_t snapshot[FONT_MISSING_LOG_LEN];

    for (int i = 0; i < FONT_MISSING_LOG_LEN; i++)
        snapshot[i] = missing_log[i];
    uint32_t saved_head = missing_head, saved_count = missing_count;

    missing_head = 0;

    missing_count = 0;
    for (int i = 0; i < FONT_MISSING_LOG_LEN + 3; i++)
        push_missing('?', i, i);

    font_missing_entry_t out[FONT_MISSING_LOG_LEN];

    uint32_t n = font_get_missing_log(out, FONT_MISSING_LOG_LEN);
    bool ok = (n == FONT_MISSING_LOG_LEN) && (out[0].x == 3) && (out[FONT_MISSING_LOG_LEN - 1].x == FONT_MISSING_LOG_LEN + 2);

    for (int i = 0; i < FONT_MISSING_LOG_LEN; i++)
        missing_log[i] = snapshot[i];

    missing_head = saved_head;
    missing_count = saved_count;

    return ok;
}
