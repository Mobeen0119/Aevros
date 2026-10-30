#include "desktop.h"
#include "style.h"
#include "../Framebuffer/framebuffer.h"
#include "../../kernel/Process/task.h"

#define SCREEN_W 1024
#define SCREEN_H 768
#define HORIZON_Y 700
#define HORIZON_ROWS 20

static void horizon(void)
{
    for (int k = 0; k < HORIZON_ROWS; k++)
    {
        uint32_t lift = (uint32_t)(HORIZON_ROWS - 1 - k);
        uint32_t r = (0x2A * lift) / (HORIZON_ROWS - 1);
        uint32_t g = (0x16 * lift) / (HORIZON_ROWS - 1);

        uint32_t b = (0x08 * lift) / (HORIZON_ROWS - 1);
        aev_fill(0, HORIZON_Y + k, SCREEN_W, 1, (r << 16) | (g << 8) | b);
    }
    aev_hline(0, HORIZON_Y, SCREEN_W, 0x6B4A1E);
}

static void top_bar(void)
{
    aev_fill(0, 0, SCREEN_W, 30, AEV_BAR);
    aev_hline(0, 30, SCREEN_W, AEV_RULE);

    aev_text(18, 7, "AEVROS", AEV_DAWN, 2);
    aev_text(170, 11, "EVERY LAYER, OWNED", AEV_MUTED, 1);

    uint32_t s = get_ticks() / 100;
    char clock[16];
    int len = 0;
    clock[0] = 0;

    len = aev_put_str(clock, len, sizeof(clock), "UP ");

    uint32_t hh = s / 3600;
    uint32_t mm = (s / 60) % 60;
    uint32_t ss = s % 60;

    if (hh < 10)
        len = aev_put_str(clock, len, sizeof(clock), "0");

    len = aev_put_uint(clock, len, sizeof(clock), hh);
    len = aev_put_str(clock, len, sizeof(clock), ":");

    if (mm < 10)
        len = aev_put_str(clock, len, sizeof(clock), "0");

    len = aev_put_uint(clock, len, sizeof(clock), mm);
    len = aev_put_str(clock, len, sizeof(clock), ":");

    if (ss < 10)
        len = aev_put_str(clock, len, sizeof(clock), "0");

    aev_put_uint(clock, len, sizeof(clock), ss);
    aev_text(SCREEN_W - 18 - (int)aev_text_width(clock, 1), 11, clock, AEV_TEXT, 1);
}

static void bottom_bar(void)
{
    aev_hline(0, SCREEN_H - 30, SCREEN_W, AEV_RULE);

    aev_text(18, SCREEN_H - 20, "ESC  SHELL / NETVIEW", AEV_MUTED, 1);
    aev_text(220, SCREEN_H - 20, "F2  DESKTOP", AEV_MUTED, 1);
}

void desktop_paint(void)
{
    aev_fill(0, 0, SCREEN_W, SCREEN_H, AEV_BG);
    horizon();

    top_bar();
    bottom_bar();
}