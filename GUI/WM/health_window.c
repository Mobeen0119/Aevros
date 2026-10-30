#include "health_window.h"
#include "style.h"
#include "../Algo/Font/font.h"
#include "../../kernel/Memory/pmm.h"
#include "../../kernel/Memory/KallocTracker/kalloc_tracker.h"
#include "../../kernel/Process/task.h"
#include "../../kernel/Shell/Shell_Health/shell_health.h"

int shell_health_task_count(void);

static void put_pad2(char *buf, int *len, int cap, uint32_t v)
{
    if (v < 10)
        *len = aev_put_str(buf, *len, cap, "0");

    *len = aev_put_uint(buf, *len, cap, v);
}

static void put_clock(char *buf, int *len, int cap, uint32_t seconds)
{
    put_pad2(buf, len, cap, seconds / 3600);
    *len = aev_put_str(buf, *len, cap, ":");

    put_pad2(buf, len, cap, (seconds / 60) % 60);
    *len = aev_put_str(buf, *len, cap, ":");

    put_pad2(buf, len, cap, seconds % 60);
}

static void put_row(int x, int y, const char *label, const char *value)
{
    aev_text(x, y, label, AEV_MUTED, 1);
    aev_text(x + 210, y, value, AEV_TEXT, 1);
}

const char *health_window_status(uint32_t *color)
{
    const char *word = shell_health();

    if (word[0] == 'S')
        *color = AEV_OK;

    else if (word[0] == 'L')
        *color = AEV_DAWN;

    else
        *color = AEV_WARN;

    return word;
}

void health_window_why(char *buf, int cap)
{
    int len = 0;
    len = aev_put_str(buf, len, cap, "FRAMES ");
    len = aev_put_uint(buf, len, cap, pmm_free_frames());

    len = aev_put_str(buf, len, cap, " FREE  ALLOCS ");
    len = aev_put_uint(buf, len, cap, tracker_live_count());

    len = aev_put_str(buf, len, cap, "  TASKS ");

    len = aev_put_uint(buf, len, cap, shell_health_task_count());
    len = aev_put_str(buf, len, cap, "  LIMITS 16/64 FRAMES 128 ALLOCS 32 TASKS");
}

void health_window_paint(int x, int y, int w, int h)
{
    (void)h;

    uint32_t total = pmm_total_frames();
    uint32_t free_frames = pmm_free_frames();
    uint32_t used_pct = total ? (total - free_frames) * 100 / total : 0;

    uint32_t status_color = AEV_TEXT;
    const char *status = health_window_status(&status_color);
    aev_text(x + 16, y + 12, status, status_color, 2);

    char value[32];
    int len;

    len = 0;

    len = aev_put_uint(value, len, sizeof(value), total - free_frames);
    len = aev_put_str(value, len, sizeof(value), " OF ");
    len = aev_put_uint(value, len, sizeof(value), total);

    aev_put_str(value, len, sizeof(value), " FRAMES");
    aev_text(x + 16, y + 56, "MEMORY IN USE", AEV_MUTED, 1);
    aev_text(x + 210, y + 56, value, AEV_TEXT, 1);
    aev_bar(x + 16, y + 74, w - 32, 8, used_pct, AEV_DAWN);

    len = 0;
    len = aev_put_uint(value, len, sizeof(value), free_frames);
    len = aev_put_str(value, len, sizeof(value), " FRAMES");

    put_row(x + 16, y + 96, "FREE", value);

    len = 0;
    len = aev_put_uint(value, len, sizeof(value), tracker_live_count());
    put_row(x + 16, y + 116, "LIVE ALLOCATIONS", value);

    len = 0;
    len = aev_put_uint(value, len, sizeof(value), shell_health_task_count());
    put_row(x + 16, y + 136, "TASKS", value);

    len = 0;
    put_clock(value, &len, sizeof(value), get_ticks() / 100);
    put_row(x + 16, y + 156, "UPTIME", value);
}
