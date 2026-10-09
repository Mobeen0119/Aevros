#include "net_view.h"
#include "../../Framebuffer/framebuffer.h"
#include "../../Algo/algo.h"
#include "../../Algo/Render/render_color.h"
#include "../../../Networking/NetTrace/net_trace.h"
#include "../../../Networking/NetWatch/net_watch.h"
#include "../../Algo/Font/font.h"

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

#define GRAPH_X 24
#define GRAPH_Y 350
#define GRAPH_W 300
#define GRAPH_H 350
#define GRAPH_HISTORY_LEN 96

#define NETVIEW_CHECK_TICKS 50

extern uint64_t get_ticks(void);

static uint8_t visible;
static uint32_t selected_id;
static uint8_t has_selection;

static netwatch_snapshot_t snapshot;

static uint64_t last_check;
static uint32_t last_pending;
static uint32_t last_timed_out;

static uint32_t rx_history[GRAPH_HISTORY_LEN];
static uint32_t tx_history[GRAPH_HISTORY_LEN];

static uint32_t history_head;
static uint32_t history_count;

static uint32_t previous_rx;
static uint32_t previous_tx;
static uint8_t have_previous_counters;

static uint32_t status_color(nettrace_status_t s);
static const char *status_label(nettrace_status_t s);

static uint32_t scale_u32(uint32_t value, uint32_t multiplier, uint32_t divisor)
{
    if (divisor == 0 || multiplier == 0)
        return 0;

    if (value >= divisor)
        return multiplier;

    uint32_t result = 0;
    uint32_t remainder = 0;

    for (uint32_t i = 0; i < multiplier; ++i)
    {
        if (remainder >= divisor - value)
        {
            remainder -= divisor - value;
            ++result;
        }
        else
        {
            remainder += value;
        }
    }

    return result;
}

static uint32_t text_color(void)
{
    return ((uint32_t)TEXT_R << 16) | ((uint32_t)TEXT_G << 8) | TEXT_B;
}

static int32_t selected_index(void)
{
    if (!has_selection)
        return -1;

    for (uint32_t i = 0;
         i < snapshot.event_count && i < NETWATCH_MAX_EVENTS;
         ++i)
    {
        if (snapshot.events[i].valid &&
            snapshot.events[i].id == selected_id)
        {
            return (int32_t)i;
        }
    }

    return -1;
}

static void drop_selection_if_gone(void)
{
    if (has_selection && selected_index() < 0)
        has_selection = 0;
}

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

    font_draw_string(24, 13, "AEVROS / NETWORK OBSERVATORY", text_color());

    font_draw_string(24, 32, "Packet activity and event inspection", (ACCENT_R << 16) | (ACCENT_G << 8) | ACCENT_B);
}

static void draw_status_indicator(int x, int y, uint8_t up)
{
    if (up)
        fb_circle_filled(x, y, 6, RX_R, RX_G, RX_B);
    else
        fb_circle_filled(x, y, 6, TX_R, TX_G, TX_B);
}

static void draw_traffic_bars(void)
{
    const int x = STATS_X + 24;
    const int base_y = STATS_Y + 202;
    const int max_h = 70;
    const int bar_w = 72;

    uint32_t rx = snapshot.stats.rx_packets;
    uint32_t tx = snapshot.stats.tx_packets;
    uint32_t max = (rx > tx) ? rx : tx;

    if (max == 0)
        max = 1;

    int rx_h = (int)scale_u32(rx, max_h, max);
    int tx_h = (int)scale_u32(tx, max_h, max);

    font_draw_string(x, STATS_Y + 150, "RX PACKETS",
                     (RX_R << 16) | (RX_G << 8) | RX_B);

    font_draw_string(x + 140, STATS_Y + 150, "TX PACKETS",
                     (TX_R << 16) | (TX_G << 8) | TX_B);

    fb_rect(x, base_y - max_h, bar_w, max_h, LINE_R, LINE_G, LINE_B);

    fb_rect(x + 140, base_y - max_h, bar_w, max_h, LINE_R, LINE_G, LINE_B);

    if (rx_h > 0)
    {
        fb_rect_filled(x + 1, base_y - rx_h, bar_w - 2, rx_h, RX_R, RX_G, RX_B);
    }

    if (tx_h > 0)
    {
        fb_rect_filled(x + 141, base_y - tx_h, bar_w - 2, tx_h, TX_R, TX_G, TX_B);
    }
}

static void draw_stats(void)
{
    draw_panel(STATS_X, STATS_Y, STATS_W, STATS_H);

    draw_status_indicator(STATS_X + 20, STATS_Y + 24, snapshot.stats.link_up);

    font_draw_string(STATS_X + 36, STATS_Y + 20, snapshot.stats.link_up ? "LINK UP" : "LINK DOWN",
                     snapshot.stats.link_up ? ((RX_R << 16) | (RX_G << 8) | RX_B) : ((TX_R << 16) | (TX_G << 8) | TX_B));

    font_draw_string(STATS_X + 20, STATS_Y + 46, "RX TOTAL", text_color());

    font_draw_string(STATS_X + 170, STATS_Y + 46, "TX TOTAL", text_color());

    // Decimal conversion without libc formatting
    char rx_text[11];
    char tx_text[11];

    uint32_t rx = snapshot.stats.rx_packets;
    uint32_t tx = snapshot.stats.tx_packets;

    for (int i = 9; i >= 0; --i)
    {
        rx_text[i] = (char)('0' + rx % 10);
        tx_text[i] = (char)('0' + tx % 10);

        rx /= 10;
        tx /= 10;
    }

    rx_text[10] = '\0';
    tx_text[10] = '\0';

    font_draw_string(STATS_X + 20, STATS_Y + 62, rx_text, (RX_R << 16) | (RX_G << 8) | RX_B);

    font_draw_string(STATS_X + 170, STATS_Y + 62, tx_text, (TX_R << 16) | (TX_G << 8) | TX_B);

    draw_traffic_bars();
}

static void draw_protocol_activity(void)
{
    const int x = STATS_X + 20;
    const int y = STATS_Y + 100;

    const uint32_t values[6] = {
        snapshot.stats.arp_packets,
        snapshot.stats.ipv4_packets,
        snapshot.stats.ipv6_packets,
        snapshot.stats.icmp_packets,
        snapshot.stats.udp_packets,
        snapshot.stats.tcp_packets};

    const char *labels[6] = {
        "ARP", "IPv4", "IPv6", "ICMP", "UDP", "TCP"};

    uint32_t max = 1;

    for (uint32_t i = 0; i < 6; ++i)
    {
        if (values[i] > max)
            max = values[i];
    }

    for (uint32_t i = 0; i < 6; ++i)
    {
        int row_y = y + (int)i * 14;

        font_draw_string(x, row_y, labels[i], text_color());

        int bar_w = (int)scale_u32(values[i], 150, max);

        if (bar_w > 0)
        {
            fb_rect_filled(x + 48, row_y + 1, bar_w, 7, ACCENT_R, ACCENT_G, ACCENT_B);
        }
    }
}

static void record_traffic_sample(const netwatch_stats_t *stats)
{
    if (!stats)
        return;

    uint32_t rx_delta = 0;
    uint32_t tx_delta = 0;

    if (have_previous_counters && stats->rx_packets >= previous_rx && stats->tx_packets >= previous_tx)
    {
        rx_delta = stats->rx_packets - previous_rx;
        tx_delta = stats->tx_packets - previous_tx;
    }

    previous_rx = stats->rx_packets;
    previous_tx = stats->tx_packets;
    have_previous_counters = 1;

    rx_history[history_head] = rx_delta;
    tx_history[history_head] = tx_delta;

    history_head = (history_head + 1) % GRAPH_HISTORY_LEN;

    if (history_count < GRAPH_HISTORY_LEN)
        ++history_count;
}

static uint32_t history_value(const uint32_t *history, uint32_t index)
{
    return history[index];
}

static void draw_traffic_history(void)
{
    draw_panel(GRAPH_X, GRAPH_Y, GRAPH_W, GRAPH_H);

    font_draw_string(GRAPH_X + 12, GRAPH_Y + 10, "PACKETS / UPDATE", text_color());

    font_draw_string(GRAPH_X + 150, GRAPH_Y + 10, "RX", (RX_R << 16) | (RX_G << 8) | RX_B);

    font_draw_string(GRAPH_X + 185, GRAPH_Y + 10, "TX", (TX_R << 16) | (TX_G << 8) | TX_B);

    const int gx = GRAPH_X + 12;
    const int gy = GRAPH_Y + 32;
    const int gw = GRAPH_W - 24;
    const int gh = GRAPH_H - 46;

    fb_rect(gx, gy, gw, gh, LINE_R, LINE_G, LINE_B);

    for (int i = 1; i < 4; ++i)
    {
        int yy = gy + (gh * i) / 4;

        fb_line(gx + 1, yy, gx + gw - 1, yy, LINE_R, LINE_G, LINE_B);
    }

    if (history_count < 2)
    {
        font_draw_string(gx + 6, gy + 6, "Waiting for samples", text_color());

        return;
    }

    uint32_t max_value = 1;

    for (uint32_t i = 0; i < history_count; ++i)
    {
        uint32_t index =
            (history_head + GRAPH_HISTORY_LEN - history_count + i) % GRAPH_HISTORY_LEN;

        if (rx_history[index] > max_value)
            max_value = rx_history[index];

        if (tx_history[index] > max_value)
            max_value = tx_history[index];
    }

    for (uint32_t i = 1; i < history_count; ++i)
    {
        uint32_t previous = (history_head + GRAPH_HISTORY_LEN - history_count + i - 1) % GRAPH_HISTORY_LEN;

        uint32_t current = (history_head + GRAPH_HISTORY_LEN - history_count + i) % GRAPH_HISTORY_LEN;

        int x0 = gx + 1 + (int)scale_u32(i - 1, (uint32_t)(gw - 2), GRAPH_HISTORY_LEN - 1);

        int x1 = gx + 1 + (int)scale_u32(i, (uint32_t)(gw - 2), GRAPH_HISTORY_LEN - 1);

        uint32_t rx0_scaled = scale_u32(history_value(rx_history, previous), (uint32_t)(gh - 4), max_value);

        uint32_t rx1_scaled = scale_u32(history_value(rx_history, current), (uint32_t)(gh - 4), max_value);

        uint32_t tx0_scaled = scale_u32(history_value(tx_history, previous), (uint32_t)(gh - 4), max_value);

        uint32_t tx1_scaled = scale_u32(history_value(tx_history, current), (uint32_t)(gh - 4), max_value);

        int rx0 = gy + gh - 2 - (int)rx0_scaled;
        int rx1 = gy + gh - 2 - (int)rx1_scaled;
        int tx0 = gy + gh - 2 - (int)tx0_scaled;
        int tx1 = gy + gh - 2 - (int)tx1_scaled;

        fb_line(x0, rx0, x1, rx1, RX_R, RX_G, RX_B);
        fb_line(x0, tx0, x1, tx1, TX_R, TX_G, TX_B);
    }
}

static const char *protocol_name(netwatch_protocol_t protocol)
{
    switch (protocol)
    {
    case NETWATCH_PROTO_ETHERNET:
        return "Ethernet";
    case NETWATCH_PROTO_ARP:
        return "ARP";
    case NETWATCH_PROTO_IPV4:
        return "IPv4";
    case NETWATCH_PROTO_IPV6:
        return "IPv6";
    case NETWATCH_PROTO_ICMP:
        return "ICMP";
    case NETWATCH_PROTO_UDP:
        return "UDP";
    case NETWATCH_PROTO_TCP:
        return "TCP";
    default:
        return "Unknown";
    }
}

static void draw_event_row(uint32_t index, int y)
{
    netwatch_event_t *event = &snapshot.events[index];

    if (!event->valid)
        return;

    if (has_selection && event->id == selected_id)
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

    font_draw_string(EVENTS_X + 42, y + 7, protocol_name(event->protocol), text_color());

    font_draw_string(EVENTS_X + 132, y + 7, event->direction == NETWATCH_RX ? "RX" : "TX",
                     event->direction == NETWATCH_RX ? ((RX_R << 16) | (RX_G << 8) | RX_B) : ((TX_R << 16) | (TX_G << 8) | TX_B));

    char length_text[11];
    uint32_t length = event->length;

    for (int i = 9; i >= 0; --i)
    {
        length_text[i] = (char)('0' + length % 10);
        length /= 10;
    }

    length_text[10] = '\0';

    font_draw_string(EVENTS_X + 200, y + 7, length_text, text_color());

    const nettrace_event_t *trace = nettrace_get_event(index);

    if (trace && trace->status != NETTRACE_STATUS_NONE)
    {
        font_draw_string(EVENTS_X + EVENTS_W - 64, y + 7, status_label(trace->status), status_color(trace->status));
    }
}

static void draw_events(void)
{
    draw_panel(EVENTS_X, EVENTS_Y, EVENTS_W, EVENTS_H);

    font_draw_string(EVENTS_X + 16, EVENTS_Y + 14, "RECENT NETWORK EVENTS", text_color());

    font_draw_string(EVENTS_X + 16, EVENTS_Y + 30, "Protocol       Dir       Length (bytes)", (ACCENT_R << 16) | (ACCENT_G << 8) | ACCENT_B);

    uint32_t count = snapshot.event_count;

    if (count > NETWATCH_MAX_EVENTS)
        count = NETWATCH_MAX_EVENTS;

    if (count > 13)
        count = 13;

    for (uint32_t i = 0; i < count; ++i)
    {
        if (!snapshot.events[i].valid)
            continue;

        draw_event_row(i, EVENTS_Y + 48 + (int)i * EVENT_ROW_HEIGHT);
    }
}

static void draw_event_detail(void)
{
    int32_t index = selected_index();

    const int x = EVENTS_X;
    const int y = 710;
    const int w = EVENTS_W;
    const int h = 44;

    draw_panel(x, y, w, h);

    if (index < 0)
    {
        font_draw_string(x + 12, y + 10, "Select an event to inspect its details.", text_color());

        return;
    }

    const netwatch_event_t *event = &snapshot.events[index];

    font_draw_string(x + 12, y + 6, protocol_name(event->protocol), (ACCENT_R << 16) | (ACCENT_G << 8) | ACCENT_B);

    font_draw_string(x + 100, y + 6, event->direction == NETWATCH_RX ? "Received packet" : "Transmitted packet", text_color());

    font_draw_string(x + 12, y + 22, "Length (bytes):", text_color());

    char length_text[11];
    uint32_t length = event->length;

    for (int i = 9; i >= 0; --i)
    {
        length_text[i] = (char)('0' + length % 10);
        length /= 10;
    }

    length_text[10] = '\0';

    font_draw_string(
        x + 112, y + 22,
        length_text, text_color());
}

void network_view_init(void)
{
    visible = 0;
    has_selection = 0;
    selected_id = 0;

    last_check = 0;
    last_pending = 0;
    last_timed_out = 0;

    history_head = 0;
    history_count = 0;
    have_previous_counters = 0;

    previous_rx = 0;
    previous_tx = 0;

    for (uint32_t i = 0; i < GRAPH_HISTORY_LEN; ++i)
    {
        rx_history[i] = 0;
        tx_history[i] = 0;
    }

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

    for (uint32_t i = 0; i < NETWATCH_MAX_EVENTS; ++i)
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

void network_view_update(const netwatch_snapshot_t *state)
{
    if (!state)
        return;

    snapshot = *state;

    record_traffic_sample(&snapshot.stats);
    nettrace_rebuild_at(&snapshot, get_ticks());
    drop_selection_if_gone();

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
    draw_traffic_history();
    draw_event_detail();
}

void network_view_mouse(int32_t x, int32_t y, uint8_t buttons)
{
    if (!visible || !(buttons & 1))
        return;

    if (x < EVENTS_X || x >= EVENTS_X + EVENTS_W)
        return;

    if (y < EVENTS_Y + 48 || y >= EVENTS_Y + 48 + 13 * EVENT_ROW_HEIGHT)
        return;

    uint32_t index = (uint32_t)((y - (EVENTS_Y + 48)) / EVENT_ROW_HEIGHT);

    if (index >= snapshot.event_count || index >= NETWATCH_MAX_EVENTS || !snapshot.events[index].valid)
        return;

    selected_id = snapshot.events[index].id;
    has_selection = 1;

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
        network_view_clear_selection();
        return;
    }
}

void network_view_select_event(uint32_t index)
{
    if (index >= snapshot.event_count || index >= NETWATCH_MAX_EVENTS || !snapshot.events[index].valid)
        return;

    selected_id = snapshot.events[index].id;
    has_selection = 1;

    if (visible)
        network_view_draw();
}

int32_t network_view_selected_event(void)
{
    return selected_index();
}

void network_view_clear_selection(void)
{
    has_selection = 0;

    if (visible)
        network_view_draw();
}

void network_view_draw_event_detail(void)
{
    if (visible)
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
    netwatch_get_snapshot(&snapshot);
}

void netview_tick(uint64_t now)
{
    if (now < last_check ||
        now - last_check < NETVIEW_CHECK_TICKS)
        return;

    last_check = now;

    uint32_t old_rx = snapshot.stats.rx_packets;
    uint32_t old_tx = snapshot.stats.tx_packets;

    fill_snapshot();

    record_traffic_sample(&snapshot.stats);
    drop_selection_if_gone();

    nettrace_rebuild_at(&snapshot, now);

    uint32_t pending = 0;
    uint32_t timed_out = 0;
    uint32_t count = nettrace_get_event_count();

    for (uint32_t i = 0; i < count; ++i)
    {
        const nettrace_event_t *event = nettrace_get_event(i);

        if (!event)
            continue;

        if (event->status == NETTRACE_STATUS_PENDING)
            ++pending;
        else if (event->status == NETTRACE_STATUS_TIMED_OUT)
            ++timed_out;
    }

    if (pending != last_pending || timed_out != last_timed_out || snapshot.stats.rx_packets != old_rx || snapshot.stats.tx_packets != old_tx)
    {
        last_pending = pending;
        last_timed_out = timed_out;

        if (visible)
            network_view_draw();
    }
}

void network_view_reset(void)
{
    visible = 0;
    has_selection = 0;
    selected_id = 0;

    snapshot.event_count = 0;

    history_head = 0;
    history_count = 0;
    have_previous_counters = 0;

    previous_rx = 0;
    previous_tx = 0;

    for (uint32_t i = 0; i < GRAPH_HISTORY_LEN; ++i)
    {
        rx_history[i] = 0;
        tx_history[i] = 0;
    }

    for (uint32_t i = 0; i < NETWATCH_MAX_EVENTS; ++i)
        snapshot.events[i].valid = 0;
}
