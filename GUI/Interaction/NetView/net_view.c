#include "net_view.h"
#include "../../Framebuffer/framebuffer.h"
#include "../../Algo/algo.h"
#include "../../Algo/Render/render_color.h"
#include "../../../Networking/NetTrace/net_trace.h"
#include "../../../Networking/NetWatch/net_watch.h"

#define BG_R 8
#define BG_G 12
#define BG_B 18

#define PANEL_R 16
#define PANEL_G 22

#define PANEL_B 30

#define LINE_R 55
#define LINE_G 65
#define LINE_B 80

#define TEXT_R 220

#define TEXT_G 225
#define TEXT_B 235

#define ACCENT_R 70
#define ACCENT_G 180
#define ACCENT_B 255

#define RX_R 70
#define RX_G 220
#define RX_B 150

#define TX_R 240
#define TX_G 170
#define TX_B 70

#define SELECT_R 240
#define SELECT_G 200
#define SELECT_B 80

#define HEADER_HEIGHT 56
#define STATS_X 24
#define STATS_Y 80

#define STATS_W 300
#define STATS_H 250

#define EVENTS_X 344
#define EVENTS_Y 80
#define EVENTS_W 656
#define EVENTS_H 620

#define EVENT_ROW_HEIGHT 42

#define NETVIEW_CHECK_TICKS 50

static uint64_t last_check = 0;
static uint32_t last_pending = 0;
static uint32_t last_timed_out = 0;

static uint8_t visible = 0;

static int32_t selected_event = -1;
static netwatch_snapshot_t snapshot;

static void fill_background(void)
{
    fb_rect_filled(0, 0, NETWORK_VIEW_WIDTH, NETWORK_VIEW_HEIGHT, BG_R, BG_G, BG_B);
}

static void draw_panel(int x, int y, int w, int h)
{
    fb_rect_filled(x, y, w, h, PANEL_R, PANEL_G, PANEL_B);

    fb_rect(x, y, w, h, LINE_R, LINE_G, LINE_B);
}

static void draw_header(void)
{
    fb_rect_filled(0, 0, NETWORK_VIEW_WIDTH, HEADER_HEIGHT, PANEL_R, PANEL_G, PANEL_B);

    fb_line(0, HEADER_HEIGHT - 1, NETWORK_VIEW_WIDTH - 1, HEADER_HEIGHT - 1, LINE_R, LINE_G, LINE_B);
}

static void draw_status_indicator(int x, int y, uint8_t up)
{
    if (up)
        fb_circle_filled(x, y, 7, RX_R, RX_G, RX_B);
    else
        fb_circle_filled(x, y, 7, TX_R, TX_G, TX_B);
}

static void draw_traffic_bars(void)
{
    uint32_t total = snapshot.stats.rx_packets + snapshot.stats.tx_packets;

    int base_x = STATS_X + 20;

    int base_y = STATS_Y + 220;
    int max_h = 80;

    uint32_t rx_h = 0;
    uint32_t tx_h = 0;

    if (total)
    {
        rx_h = (snapshot.stats.rx_packets * max_h) / total;

        tx_h = (snapshot.stats.tx_packets * max_h) / total;
    }

    fb_rect_filled(base_x, base_y - rx_h, 80, rx_h, RX_R, RX_G, RX_B);

    fb_rect_filled(base_x + 120, base_y - tx_h, 80, tx_h, TX_R, TX_G, TX_B);

    fb_rect(base_x, base_y - max_h, 80, max_h, LINE_R, LINE_G, LINE_B);
    fb_rect(base_x + 120, base_y - max_h, 80, max_h, LINE_R, LINE_G, LINE_B);
}

static void draw_stats(void)
{
    draw_panel(STATS_X, STATS_Y, STATS_W, STATS_H);

    draw_status_indicator(STATS_X + 24, STATS_Y + 24, snapshot.stats.link_up);
    draw_traffic_bars();
}

static void draw_event_row(uint32_t index, int y)
{
    netwatch_event_t *event = &snapshot.events[index];

    uint8_t selected = ((int32_t)index == selected_event);

    if (selected)
    {
        fb_rect_filled(EVENTS_X + 8, y, EVENTS_W - 16, EVENT_ROW_HEIGHT, 35, 42, 52);
        fb_rect(EVENTS_X + 8, y, EVENTS_W - 16, EVENT_ROW_HEIGHT, SELECT_R, SELECT_G, SELECT_B);
    }
    else
    {
        fb_line(EVENTS_X + 16, y + EVENT_ROW_HEIGHT - 1, EVENTS_X + EVENTS_W - 16, y + EVENT_ROW_HEIGHT - 1, LINE_R, LINE_G, LINE_B);
    }

    if (event->direction == NETWATCH_RX)
        fb_circle_filled(EVENTS_X + 26, y + 21, 5, RX_R, RX_G, RX_B);
    else
        fb_circle_filled(EVENTS_X + 26, y + 21, 5, TX_R, TX_G, TX_B);
}

static void draw_events(void)
{
    draw_panel(EVENTS_X, EVENTS_Y, EVENTS_W, EVENTS_H);

    uint32_t count = snapshot.event_count;

    if (count > NETWORK_VIEW_MAX_EVENTS)
        count = NETWORK_VIEW_MAX_EVENTS;

    for (uint32_t i = 0; i < count; i++)
    {
        if (!snapshot.events[i].valid)
            continue;

        draw_event_row(i, EVENTS_Y + 48 + (i * EVENT_ROW_HEIGHT));
    }
}

static void draw_event_detail(void)
{
    if (selected_event < 0)
        return;

    if ((uint32_t)selected_event >= snapshot.event_count)
        return;

    netwatch_event_t *event = &snapshot.events[selected_event];

    int x = 24;
    int y = 350;

    int w = 300;
    int h = 350;

    draw_panel(x, y, w, h);

    fb_rect_filled(x + 12, y + 12, w - 24, 4, ACCENT_R, ACCENT_G, ACCENT_B);

    (void)event;
}

static void draw_protocol_activity(void)
{
    int x = STATS_X + 20;
    int y = STATS_Y + 115;

    uint32_t values[6];

    values[0] = snapshot.stats.arp_packets;
    values[1] = snapshot.stats.ipv4_packets;

    values[2] = snapshot.stats.ipv6_packets;
    values[3] = snapshot.stats.icmp_packets;

    values[4] = snapshot.stats.udp_packets;
    values[5] = snapshot.stats.tcp_packets;

    uint32_t max = 1;

    for (uint32_t i = 0; i < 6; i++)
    {
        if (values[i] > max)
            max = values[i];
    }

    for (uint32_t i = 0; i < 6; i++)
    {
        int bar = (int)((values[i] * 180) / max);

        fb_rect_filled(x, y + (i * 14), bar, 8, ACCENT_R, ACCENT_G, ACCENT_B);
    }
}

void network_view_init(void)
{
    visible = 0;
    selected_event = -1;

    snapshot.stats.rx_packets = 0;

    snapshot.stats.tx_packets = 0;

    snapshot.stats.rx_bytes = 0;
    snapshot.stats.tx_bytes = 0;

    snapshot.stats.arp_packets = 0;
    snapshot.stats.ipv4_packets = 0;

    snapshot.stats.ipv6_packets = 0;
    snapshot.stats.icmp_packets = 0;

    snapshot.stats.udp_packets = 0;
    snapshot.stats.tcp_packets = 0;

    snapshot.stats.link_up = 0;
    snapshot.event_count = 0;

    for (uint32_t i = 0; i < NETWATCH_MAX_EVENTS; i++)
        snapshot.events[i].valid = 0;
}

void network_view_show(void)
{
    visible = 1;

    network_view_draw();
}

void network_view_hide(void)
{
    visible = 0;
}

void network_view_toggle(void)
{
    if (visible)
        network_view_hide();
    else
        network_view_show();
}

uint8_t network_view_is_visible(void)
{
    return visible;
}

panel_rect_t netview_panel_rect(void)
{
    panel_rect_t r;
    r.x = 216;
    r.y = 8;
    r.w = 300;

    r.h = 200;
    return r;
}

void netview_handle_click(int32_t x, int32_t y)
{
    (void)x;
    (void)y;
}

void network_view_update(const netwatch_snapshot_t *state)
{
    if (!state)
        return;

    snapshot = *state;

    if (selected_event >= 0 &&
        (uint32_t)selected_event >= snapshot.event_count)
    {
        selected_event = -1;
    }

    if (visible)
        network_view_draw();
}

void network_view_draw(void)
{
    if (!visible)
        return;

    fill_background();

    draw_header();
    draw_stats();

    draw_protocol_activity();

    draw_events();
    draw_event_detail();
}

void network_view_mouse(int32_t x, int32_t y, uint8_t buttons)
{
    if (!visible)
        return;

    if (!(buttons & 1))
        return;

    if (x < EVENTS_X || x >= EVENTS_X + EVENTS_W)
        return;

    if (y < EVENTS_Y + 48)
        return;

    uint32_t index = (uint32_t)((y - (EVENTS_Y + 48)) / EVENT_ROW_HEIGHT);

    if (index >= snapshot.event_count)
        return;

    if (!snapshot.events[index].valid)
        return;

    selected_event = (int32_t)index;

    network_view_draw();
}

void network_view_key(uint8_t key)
{
    if (!visible)
        return;

    if (key == 27)
    {
        network_view_hide();
        return;
    }

    if (key == 'r' || key == 'R')
    {
        selected_event = -1;
        network_view_draw();
        return;
    }
}

void network_view_select_event(uint32_t index)
{
    if (index >= snapshot.event_count)
        return;

    if (!snapshot.events[index].valid)
        return;

    selected_event = (int32_t)index;

    if (visible)
        network_view_draw();
}

int32_t network_view_selected_event(void)
{
    return selected_event;
}

void network_view_clear_selection(void)
{
    selected_event = -1;

    if (visible)
        network_view_draw();
}

void network_view_draw_event_detail(void)
{
    draw_event_detail();
}

static uint32_t status_color(nettrace_status_t s)
{
    switch (s)
    {
    case NETTRACE_STATUS_ANSWERED:
        return COLOR_VERIFIED;

    case NETTRACE_STATUS_PENDING:
        return COLOR_UNVERIFIED;

    case NETTRACE_STATUS_TIMED_OUT:
        return COLOR_ORPHANED;

    default:
        return COLOR_TITLE_TEXT;
    }
}

static const char *status_label(nettrace_status_t s)
{
    switch (s)
    {
    case NETTRACE_STATUS_ANSWERED:
        return "ok";

    case NETTRACE_STATUS_PENDING:
        return "wait";

    case NETTRACE_STATUS_TIMED_OUT:
        return "LOST";

    default:
        return "";
    }
}

static void fill_snapshot(void)
{
    uint32_t n = netwatch_get_event_count();

    if (n > NETWATCH_MAX_EVENTS)
        n = NETWATCH_MAX_EVENTS;

    for (uint32_t i = 0; i < n; i++)
    {
        const netwatch_event_t *e = netwatch_get_event(i);

        if (!e)
            break;
        snapshot.events[i] = *e;
    }

    snapshot.event_count = n;
}

void netview_tick(uint64_t now)
{
    if (now - last_check < NETVIEW_CHECK_TICKS)
        return;

    last_check = now;

    fill_snapshot();
    nettrace_rebuild_at(&snapshot, now);

    uint32_t pending = 0;

    uint32_t timed_out = 0;
    uint32_t count = nettrace_get_event_count();

    for (uint32_t i = 0; i < count; i++)
    {
        const nettrace_event_t *e = nettrace_get_event(i);

        if (!e)
            continue;

        if (e->status == NETTRACE_STATUS_PENDING)
            pending++;
        else if (e->status == NETTRACE_STATUS_TIMED_OUT)

            timed_out++;
    }

    if (pending != last_pending || timed_out != last_timed_out)
    {
        last_pending = pending;
        last_timed_out = timed_out;
    }
}

void network_view_reset(void)
{
    visible = 0;

    selected_event = -1;

    snapshot.event_count = 0;

    for (uint32_t i = 0; i < NETWATCH_MAX_EVENTS; i++)
        snapshot.events[i].valid = 0;
}
