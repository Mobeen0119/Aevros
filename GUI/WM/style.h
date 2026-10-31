#ifndef STYLE_H
#define STYLE_H

#include <stdint.h>
#include <stdbool.h>

#define AEV_BG 0x07090D
#define AEV_BAR 0x090C11
#define AEV_PANEL 0x0E1218
#define AEV_PANEL_HEAD 0x131A23
#define AEV_PANEL_FOOT 0x0A0D12
#define AEV_RULE 0x1E2631
#define AEV_RULE_DIM 0x151B24
#define AEV_DAWN 0xE8A33D
#define AEV_EMBER 0xC2572A
#define AEV_TEXT 0xE6E9EF
#define AEV_MUTED 0x7D8795
#define AEV_OK 0x5BD39A
#define AEV_WARN 0xE06060

#define AEV_HEAD_H 26
#define AEV_FOOT_H 22

void aev_fill(int x, int y, int w, int h, uint32_t c);

void aev_hline(int x, int y, int w, uint32_t c);

void aev_vline(int x, int y, int h, uint32_t c);

void aev_text(int x, int y, const char *s, uint32_t c, int scale);

uint32_t aev_text_width(const char *s, int scale);

void aev_panel(int x, int y, int w, int h, bool focused);

void aev_frame(int x, int y, int w, int h, bool focused);

void aev_bar(int x, int y, int w, int h, uint32_t percent, uint32_t c);

void aev_cross(int x, int y, int size, uint32_t c);

int aev_put_str(char *buf, int len, int cap, const char *s);

int aev_put_uint(char *buf, int len, int cap, uint32_t value);

#endif