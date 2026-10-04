#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "GUI/Interaction/NetView/net_view.h"
#include "Networking/NetWatch/net_watch.h"
#include "Networking/NetTrace/net_trace.h"

static uint64_t now_tick = 0;
uint64_t get_ticks(void) { return now_tick; }

typedef struct
{
    char s[16];
    uint32_t color;

} call_t;
static call_t calls[512];

static int ncalls = 0;

static int draws = 0;

void font_draw_string(int32_t x, int32_t y, const char *s, uint32_t c)
{
    (void)x;
    (void)y;

    if (ncalls < 512)
    {
        strncpy(calls[ncalls].s, s, 15);
        calls[ncalls].s[15] = 0;

        calls[ncalls].color = c;
        ncalls++;
    }
}
void fb_line(int a, int b, int c, int d, uint8_t r, uint8_t g, uint8_t bl)
{
    (void)a;
    (void)b;
    (void)c;

    (void)d;
    (void)r;
    (void)g;

    (void)bl;
}
void fb_rect(int a, int b, int c, int d, uint8_t r, uint8_t g, uint8_t bl)
{
    (void)a;
    (void)b;
    (void)c;

    (void)d;
    (void)r;
    (void)g;

    (void)bl;
}
void fb_rect_filled(int a, int b, int c, int d, uint8_t r, uint8_t g, uint8_t bl)
{
    (void)a;
    (void)b;
    (void)c;

    (void)d;
    (void)r;
    (void)g;

    (void)bl;
    if (a == 0 && b == 0 && c == NETWORK_VIEW_WIDTH && d == NETWORK_VIEW_HEIGHT)
        draws++;
}
void fb_circle(int a, int b, int c, uint8_t r, uint8_t g, uint8_t bl)
{
    (void)a;
    (void)b;
    (void)c;

    (void)r;
    (void)g;
    (void)bl;
}
void fb_circle_filled(int a, int b, int c, uint8_t r, uint8_t g, uint8_t bl)
{
    (void)a;
    (void)b;
    (void)c;
    (void)r;

    (void)g;
    (void)bl;
}

static int fails = 0;

static void check(const char *n, int ok)
{

    printf("%s: %s\n", ok ? "PASS" : "FAIL", n);
    if (!ok)
        fails++;
}
static int saw(const char *label, uint32_t color)
{
    for (int i = 0; i < ncalls; i++)
        if (!strcmp(calls[i].s, label) && calls[i].color == color)
            return 1;

    return 0;
}
static void reset_rec(void)
{
    ncalls = 0;
    draws = 0;
}
#define A 0x0A000001u

#define B 0x0A000002u

int main(void)
{
    netwatch_init();
    nettrace_init();

    network_view_init();
    network_view_show();

    now_tick = 1000;
    netwatch_tcp_event(NETWATCH_TX, 60, A, B, 1000, 80);
    reset_rec();

    netview_tick(1100);

    check("request 100 ticks old shows 'wait' (not LOST)", saw("wait", 0x7C7E82) && !saw("LOST", 0xB8483F));

    reset_rec();
    netview_tick(1000 + NETTRACE_TIMEOUT_TICKS + 60);

    check("request past timeout shows LOST in orphan color", saw("LOST", 0xB8483F));
    check("timeout caused exactly one redraw", draws == 1);

    reset_rec();
    netview_tick(1000 + NETTRACE_TIMEOUT_TICKS + 120);
    check("no change -> no redraw", draws == 0);

    // answered request
    netwatch_clear_events();
    network_view_reset();
    network_view_show();
    now_tick = 2000;

    netwatch_tcp_event(NETWATCH_TX, 60, A, B, 1000, 80);
    now_tick = 2010;

    netwatch_tcp_event(NETWATCH_RX, 60, B, A, 80, 1000);
    reset_rec();

    netview_tick(2100 + 1000);
    check("answered request shows 'ok' in verified color", saw("ok", 0x6FA3C2));

    printf("%d failure(s)\n", fails);
    return fails;
}
#include <stdlib.h>
void *kmalloc_raw(size_t n) { return malloc(n); }