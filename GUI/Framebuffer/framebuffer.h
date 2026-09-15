#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H
#include <stdint.h>

#define MULTIBOOT_BOOTLOADER_MAGIC 0x36D76289

int framebuffer_init(uint32_t mb_magic, uint32_t mb_info_addr);

int framebuffer_available(void);
uint32_t framebuffer_width(void);
uint32_t framebuffer_height(void);

void fb_put_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);
void fb_clear(uint8_t r, uint8_t g, uint8_t b);

#endif