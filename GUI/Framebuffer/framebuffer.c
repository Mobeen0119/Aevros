#include "framebuffer.h"
#include "../../Lib/kprintf.h"
#include "../../kernel/Paging/paging.h"

// Multiboot2's info structure is a sequence of variable-length, 8-byte aligned tags....looking for the framebuffer tag (type 8).
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
    uint8_t reserved[2];
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
        kprintf("[Framebuffer] not booted by a multiboot2-compliant loader, no graphics\n");
        return 0;
    }

    uint32_t total_size = *(const uint32_t *)(uintptr_t)mb_info_addr;
    uint8_t *ptr = (uint8_t *)(uintptr_t)(mb_info_addr + 8);
    uint8_t *end = (uint8_t *)(uintptr_t)(mb_info_addr + total_size);

    const mb2_tag_framebuffer_t *fb_tag = 0;

    while (ptr < end)
    {
        const mb2_tag_header_t *tag = (const mb2_tag_header_t *)ptr;

        if (tag->type == MB2_TAG_END)
            break;

        if (tag->type == MB2_TAG_FRAMEBUFFER)
            fb_tag = (const mb2_tag_framebuffer_t *)ptr;

        uint32_t advance = (tag->size + 7) & ~7u;
        if (advance == 0)
            break;
        ptr += advance;
    }

    if (!fb_tag)
    {
        kprintf("[Framebuffer] no framebuffer tag in multiboot2 info - GRUB may not have granted the requested mode\n");
        return 0;
    }

    if (fb_tag->framebuffer_type != FB_TYPE_RGB)
    {
        kprintf("[Framebuffer] got a non-RGB framebuffer (type %d), no graphics\n", fb_tag->framebuffer_type);
        return 0;
    }

    if (fb_tag->framebuffer_bpp != 32 && fb_tag->framebuffer_bpp != 24)
    {
        kprintf("[Framebuffer] unsupported bit depth %d bpp, no graphics\n", fb_tag->framebuffer_bpp);
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

    // The framebuffer physically lives way outside the first 40MB , so appears when paging enables

    uint32_t fb_size = fb_pitch * fb_height;
    uint32_t start_page = fb_addr & ~0xFFF;

    uint32_t end_page = (fb_addr + fb_size + 0xFFF) & ~0xFFF;
    for (uint32_t addr = start_page; addr < end_page; addr += 0x1000)
        map_page(addr, addr, PAGE_PRESENT | PAGE_WRITE );

    fb_base = (uint8_t *)(uintptr_t)fb_addr;
    available = 1;

    kprintf("[Framebuffer] ready: %ux%u @ %u bpp\n", fb_width, fb_height, fb_bpp);
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