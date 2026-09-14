#include "framebuffer.h"
#include "../../../Lib/kprintf.h"
#include "../../../kernel/Paging/paging.h"


typedef struct
{
    uint32_t type;
    uint32_t size;
} __attribute__((packed)) mb2_tag_header_t;

typedef struct
{
    uint32_t type; // = 8
    uint32_t size;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint8_t reserved[2]; //off by one shifted every color field follows
    uint8_t red_field_position, red_mask_size;
    uint8_t green_field_position, green_mask_size;
    uint8_t blue_field_position, blue_mask_size;
} __attribute__((packed)) mb2_tag_framebuffer_t;

#define MB2_TAG_END 0
#define MB2_TAG_FRAMEBUFFER 8
#define FB_TYPE_RGB 1

static uint8_t *fb_base;
static uint32_t fb_pitch, fb_width, fb_height, fb_bpp;
static uint8_t red_pos, red_size, green_pos, green_size, blue_pos, blue_size;
static int available;

int framebuffer_init(uint32_t mb_magic, uint32_t mb_info_addr)
{
    if (mb_magic != MULTIBOOT_BOOTLOADER_MAGIC)
    {
        kprintf("[Framebuffer] not booted by a multiboot2-compliant loader (magic=%x), no graphics\n", mb_magic);
        return 0;
    }

    uint32_t total_size = *(const uint32_t *)(uintptr_t)mb_info_addr;
    kprintf("[Framebuffer] mb2 info total_size=%u\n", total_size);

    uint8_t *ptr = (uint8_t *)(uintptr_t)(mb_info_addr + 8); // skip total_size(4) + reserved(4)
    uint8_t *end = (uint8_t *)(uintptr_t)(mb_info_addr + total_size);

    const mb2_tag_framebuffer_t *fb_tag = 0;

    while (ptr < end)
    {
        const mb2_tag_header_t *tag = (const mb2_tag_header_t *)ptr;

        if (tag->type == MB2_TAG_END)
            break;

        if (tag->type == MB2_TAG_FRAMEBUFFER)
            fb_tag = (const mb2_tag_framebuffer_t *)ptr;

        uint32_t advance = (tag->size + 7) & ~7u; // tags are 8-byte aligned, size itself isn't pre-padded
        if (advance == 0)
            break; // malformed tag - bail rather than loop forever
        ptr += advance;
    }

    if (!fb_tag)
    {
        kprintf("[Framebuffer] no framebuffer tag in multiboot2 info - GRUB may not have granted the requested mode\n");
        return 0;
    }

    kprintf("[Framebuffer] type=%d bpp=%d addr=%x pitch=%d %ux%u\n",
            fb_tag->framebuffer_type, fb_tag->framebuffer_bpp, (uint32_t)fb_tag->framebuffer_addr,
            fb_tag->framebuffer_pitch, fb_tag->framebuffer_width, fb_tag->framebuffer_height);

    if (fb_tag->framebuffer_type != FB_TYPE_RGB)
    {
        kprintf("[Framebuffer] got a non-RGB framebuffer (type %d) - not handling indexed/EGA modes, no graphics\n", fb_tag->framebuffer_type);
        return 0;
    }

    if (fb_tag->framebuffer_bpp != 32 && fb_tag->framebuffer_bpp != 24)
    {
        kprintf("[Framebuffer] unsupported bit depth %d bpp - only 24/32 are handled, no graphics\n", fb_tag->framebuffer_bpp);
        return 0;
    }

    uint32_t fb_addr = (uint32_t)fb_tag->framebuffer_addr;

    fb_pitch = fb_tag->framebuffer_pitch;
    fb_width = fb_tag->framebuffer_width;
    fb_height = fb_tag->framebuffer_height;
    fb_bpp = fb_tag->framebuffer_bpp;

    red_pos = fb_tag->red_field_position;
    red_size = fb_tag->red_mask_size;
    green_pos = fb_tag->green_field_position;
    green_size = fb_tag->green_mask_size;
    blue_pos = fb_tag->blue_field_position;
    blue_size = fb_tag->blue_mask_size;

    kprintf("[Framebuffer] color fields: red(pos=%d,size=%d) green(pos=%d,size=%d) blue(pos=%d,size=%d)\n",
            red_pos, red_size, green_pos, green_size, blue_pos, blue_size);


    uint32_t fb_size = fb_pitch * fb_height;
    uint32_t start_page = fb_addr & ~0xFFF;
    uint32_t end_page = (fb_addr + fb_size + 0xFFF) & ~0xFFF;
    for (uint32_t addr = start_page; addr < end_page; addr += 0x1000)
        map_page(addr, addr, PAGE_PRESENT | PAGE_WRITE);

    fb_base = (uint8_t *)(uintptr_t)fb_addr;
    available = 1;

    kprintf("[Framebuffer] ready: %ux%u @ %u bpp, pitch=%u, addr=%x\n", fb_width, fb_height, fb_bpp, fb_pitch, fb_addr);
    return 1;
}

int framebuffer_available(void)
{
    return available;
}

uint32_t framebuffer_width(void)
{
    return fb_width;
}

uint32_t framebuffer_height(void)
{
    return fb_height;
}

static inline uint32_t pack_color(uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t rr = (r >> (8 - red_size)) << red_pos;
    uint32_t gg = (g >> (8 - green_size)) << green_pos;
    uint32_t bb = (b >> (8 - blue_size)) << blue_pos;
    return rr | gg | bb;
}

void fb_put_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
    if (!available || x < 0 || y < 0 || (uint32_t)x >= fb_width || (uint32_t)y >= fb_height)
        return;

    uint32_t color = pack_color(r, g, b);
    uint8_t *p = fb_base + (uint32_t)y * fb_pitch + (uint32_t)x * (fb_bpp / 8);

    p[0] = (uint8_t)(color & 0xFF);
    p[1] = (uint8_t)((color >> 8) & 0xFF);
    p[2] = (uint8_t)((color >> 16) & 0xFF);
    if (fb_bpp == 32)
        p[3] = (uint8_t)((color >> 24) & 0xFF);
}

void fb_clear(uint8_t r, uint8_t g, uint8_t b)
{
    for (uint32_t y = 0; y < fb_height; y++)
        for (uint32_t x = 0; x < fb_width; x++)
            fb_put_pixel((int)x, (int)y, r, g, b);
}

// Bresenham, integer-only, all eight octants via the error-term/sign approach
void fb_line(int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b)
{
    int dx = x1 - x0;
    int dy = y1 - y0;
    int abs_dx = dx < 0 ? -dx : dx;
    int abs_dy = dy < 0 ? -dy : dy;
    int sx = dx < 0 ? -1 : 1;
    int sy = dy < 0 ? -1 : 1;
    int err = (abs_dx > abs_dy ? abs_dx : -abs_dy) / 2;

    int x = x0, y = y0;
    for (;;)
    {
        fb_put_pixel(x, y, r, g, b);
        if (x == x1 && y == y1)
            break;

        int e2 = err;
        if (e2 > -abs_dx)
        {
            err -= abs_dy;
            x += sx;
        }
        if (e2 < abs_dy)
        {
            err += abs_dx;
            y += sy;
        }
    }
}

void fb_rect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b)
{
    fb_line(x, y, x + w - 1, y, r, g, b);
    fb_line(x, y + h - 1, x + w - 1, y + h - 1, r, g, b);
    fb_line(x, y, x, y + h - 1, r, g, b);
    fb_line(x + w - 1, y, x + w - 1, y + h - 1, r, g, b);
}

void fb_rect_filled(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b)
{
    for (int row = 0; row < h; row++)
        fb_line(x, y + row, x + w - 1, y + row, r, g, b);
}

void fb_circle(int cx, int cy, int radius, uint8_t r, uint8_t g, uint8_t b)
{
    int x = radius;
    int y = 0;
    int err = 0;

    while (x >= y)
    {
        fb_put_pixel(cx + x, cy + y, r, g, b);
        fb_put_pixel(cx + y, cy + x, r, g, b);
        fb_put_pixel(cx - y, cy + x, r, g, b);
        fb_put_pixel(cx - x, cy + y, r, g, b);
        fb_put_pixel(cx - x, cy - y, r, g, b);
        fb_put_pixel(cx - y, cy - x, r, g, b);
        fb_put_pixel(cx + y, cy - x, r, g, b);
        fb_put_pixel(cx + x, cy - y, r, g, b);

        y += 1;
        err += 1 + 2 * y;
        if (2 * err - 2 * x + 1 > 0)
        {
            x -= 1;
            err += 1 - 2 * x;
        }
    }
}

void fb_circle_filled(int cx, int cy, int radius, uint8_t r, uint8_t g, uint8_t b)
{
    for (int dy = -radius; dy <= radius; dy++)
    {
        int dx = radius * radius - dy * dy; // dx^2 <= radius^2 - dy^2  span width
        int span = 0;
        while (span * span <= dx)
            span++;
        span--;
        fb_line(cx - span, cy + dy, cx + span, cy + dy, r, g, b);
    }
}

void fb_debug_raw_fill(uint8_t value)
{
    if (!available)
        return;

    uint32_t total_bytes = fb_pitch * fb_height;
    for (uint32_t i = 0; i < total_bytes; i++)
        fb_base[i] = value;
}