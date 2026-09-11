#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H
#include <stdint.h>

#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

int framebuffer_init(uint32_t mb_magic, uint32_t mb_info_addr);

int framebuffer_available(void);
uint32_t framebuffer_width(void);
uint32_t framebuffer_height(void);

void fb_put_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);

void fb_clear(uint8_t r, uint8_t g, uint8_t b);

void fb_line(int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b);

void fb_rect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b);
void fb_rect_filled(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b);
void fb_circle(int cx, int cy, int radius, uint8_t r, uint8_t g, uint8_t b);
void fb_circle_filled(int cx, int cy, int radius, uint8_t r, uint8_t g, uint8_t b);

#endif