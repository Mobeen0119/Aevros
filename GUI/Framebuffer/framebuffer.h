#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H
#include <stdint.h>

#define MULTIBOOT_BOOTLOADER_MAGIC 0x36D76289

int framebuffer_init(uint32_t mb_magic, uint32_t mb_info_addr);

int framebuffer_available(void);
uint32_t framebuffer_width(void);
uint32_t framebuffer_height(void);

void fb_put_pixel(int32_t x, int32_t y, uint32_t color);
void fb_put_pixel_rgb(int x, int y, uint8_t r, uint8_t g, uint8_t b);
uint32_t fb_get_pixel(int32_t x, int32_t y);

uint32_t fb_width(void);
uint32_t fb_height(void);

void fb_clear(uint8_t r, uint8_t g, uint8_t b);

#endif