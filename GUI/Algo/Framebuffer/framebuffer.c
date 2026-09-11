#include "framebuffer.h"
#include "../../../Lib/kprintf.h"
#include "../../../kernel/Paging/paging.h"

typedef struct
{
    uint32_t flags;
    uint32_t mem_lower, mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count, mods_addr;
    uint32_t syms[3];

    uint32_t mmap_length, mmap_addr;
    uint32_t drives_length, drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;

    uint32_t apm_table;
    uint32_t vbe_control_info, vbe_mode_info;
    uint16_t vbe_mode, vbe_interface_seg, vbe_interface_off, vbe_interface_len;
    uint64_t framebuffer_addr;
    
    uint32_t framebuffer_pitch;

    uint32_t framebuffer_width, framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;

    uint8_t red_field_position, red_mask_size;
    uint8_t green_field_position, green_mask_size;
    uint8_t blue_field_position, blue_mask_size;

} __attribute__((packed)) multiboot_info_t;

#define MB_FLAG_FRAMEBUFFER (1 << 12)
#define FB_TYPE_RGB 1

static uint8_t *fb_base;
static uint32_t fb_pitch, fb_width, fb_height, fb_bpp;
static uint8_t red_pos, red_size, green_pos, green_size, blue_pos, blue_size;
static int available;

int framebuffer_init(uint32_t mb_magic, uint32_t mb_info_addr)
{
    if (mb_magic != MULTIBOOT_BOOTLOADER_MAGIC)
    {
        kprintf("[Framebuffer] not booted by a multiboot-compliant loader, no graphics\n");
        return 0;
    }

    const multiboot_info_t *mbi = (const multiboot_info_t *)(uintptr_t)mb_info_addr;

    if (!(mbi->flags & MB_FLAG_FRAMEBUFFER))
    {
        kprintf("[Framebuffer] bootloader didn't hand back framebuffer info - GRUB may need updating, or the requested mode wasn't available\n");
        return 0;
    }

    if (mbi->framebuffer_type != FB_TYPE_RGB)
    {
        kprintf("[Framebuffer] got a non-RGB framebuffer (type %d) - not handling indexed/EGA modes, no graphics\n", mbi->framebuffer_type);
        return 0;
    }

    if (mbi->framebuffer_bpp != 32 && mbi->framebuffer_bpp != 24)
    {
        kprintf("[Framebuffer] unsupported bit depth %d bpp - only 24/32 are handled, no graphics\n", mbi->framebuffer_bpp);
        return 0;
    }

    fb_base = (uint8_t *)(uintptr_t)mbi->framebuffer_addr; // 32-bit kernel, high dword of the addr is always 0 
    fb_pitch = mbi->framebuffer_pitch;
    fb_width = mbi->framebuffer_width;
    fb_height = mbi->framebuffer_height;
    fb_bpp = mbi->framebuffer_bpp;

    red_pos = mbi->red_field_position;
    red_size = mbi->red_mask_size;
   
    green_pos = mbi->green_field_position;
    green_size = mbi->green_mask_size;
    blue_pos = mbi->blue_field_position;
    blue_size = mbi->blue_mask_size;

    available = 1;

    uint32_t fb_size = fb_pitch * fb_height;
    uint32_t start_page = (uint32_t)(uintptr_t)fb_base & ~0xFFF;
    uint32_t end_page = ((uint32_t)(uintptr_t)fb_base + fb_size + 0xFFF) & ~0xFFF;
    for (uint32_t addr = start_page; addr < end_page; addr += 0x1000)
        map_page(addr, addr, PAGE_PRESENT | PAGE_WRITE);

    kprintf("[Framebuffer] %ux%u @ %u bpp, pitch=%u, addr=%x\n", fb_width, fb_height, fb_bpp, fb_pitch, (uint32_t)(uintptr_t)fb_base);
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
        int dx = radius * radius - dy * dy; 
        int span = 0;
        while (span * span <= dx)
            span++;
        span--;
        fb_line(cx - span, cy + dy, cx + span, cy + dy, r, g, b);
    }
}