#include "style.h"
#include "../Algo/algo.h"
#include "../Algo/Font/font.h"

static void split(uint32_t c, uint8_t *r, uint8_t *g, uint8_t *b)
{
    *r = (uint8_t)((c >> 16) & 0xFF);
    *g = (uint8_t)((c >> 8) & 0xFF);
    *b = (uint8_t)(c & 0xFF);
}

void aev_fill(int x, int y, int w, int h, uint32_t c)
{
    uint8_t r, g, b;
    split(c, &r, &g, &b);
    fb_rect_filled(x, y, w, h, r, g, b);
}

void aev_hline(int x, int y, int w, uint32_t c)
{
    aev_fill(x, y, w, 1, c);
}

void aev_vline(int x, int y, int h, uint32_t c)
{
    aev_fill(x, y, 1, h, c);
}

void aev_text(int x, int y, const char *s, uint32_t c, int scale)
{
    if (scale <= 1)
    {
        font_draw_string(x, y, s, c);
        return;
    }

    int cx = x;
    for (const char *p = s; *p; p++)
    {
        font_draw_char_scaled(cx, y, *p, c, scale);
        cx += (FONT_GLYPH_W + 1) * scale;
    }
}

uint32_t aev_text_width(const char *s, int scale)
{
    uint32_t n = 0;
    for (const char *p = s; *p; p++)
        n++;
    return n * (FONT_GLYPH_W + 1) * (uint32_t)(scale < 1 ? 1 : scale);
}

void aev_panel(int x, int y, int w, int h, bool focused)
{
    uint32_t accent = focused ? AEV_DAWN : AEV_MUTED;
    int arm = focused ? 14 : 8;
    int thick = focused ? 2 : 1;

    aev_fill(x, y, w, h, AEV_PANEL);
    aev_fill(x, y, w, AEV_HEAD_H, AEV_PANEL_HEAD);
    aev_fill(x, y + h - AEV_FOOT_H, w, AEV_FOOT_H, AEV_PANEL_FOOT);
    aev_hline(x, y + AEV_HEAD_H, w, AEV_RULE);
    aev_hline(x, y + h - AEV_FOOT_H, w, AEV_RULE);
    aev_fill(x, y, 1, h, AEV_RULE);
    aev_fill(x + w - 1, y, 1, h, AEV_RULE);

    aev_fill(x, y, arm, thick, accent);
    aev_fill(x, y, thick, arm, accent);
    aev_fill(x + w - arm, y, arm, thick, accent);
    aev_fill(x + w - thick, y, thick, arm, accent);
    aev_fill(x, y + h - thick, arm, thick, accent);
    aev_fill(x, y + h - arm, thick, arm, accent);
    aev_fill(x + w - arm, y + h - thick, arm, thick, accent);
    aev_fill(x + w - thick, y + h - arm, thick, arm, accent);
}

void aev_bar(int x, int y, int w, int h, uint32_t percent, uint32_t c)
{
    if (percent > 100)
        percent = 100;

    aev_fill(x, y, w, h, AEV_RULE_DIM);
    aev_fill(x, y, (int)((uint32_t)w * percent / 100), h, c);
}

void aev_cross(int x, int y, int size, uint32_t c)
{
    uint8_t r, g, b;
    split(c, &r, &g, &b);
    fb_line(x, y, x + size, y + size, r, g, b);
    fb_line(x + size, y, x, y + size, r, g, b);
}

int aev_put_str(char *buf, int len, int cap, const char *s)
{
    while (*s && len < cap - 1)
        buf[len++] = *s++;
    buf[len] = 0;
    return len;
}

int aev_put_uint(char *buf, int len, int cap, uint32_t value)
{
    char tmp[12];
    int n = 0;

    do
    {
        tmp[n++] = (char)('0' + value % 10);
        value /= 10;
    } while (value);

    while (n && len < cap - 1)
        buf[len++] = tmp[--n];
    buf[len] = 0;
    return len;
}