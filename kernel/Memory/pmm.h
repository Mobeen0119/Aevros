#pragma once
#include <stdint.h>


#define MAX_FRAMES 32768

extern uint32_t mem[1024]; // 4KB of memory

void *memset(void *dest, uint8_t value, uint32_t size);
void *memcpy(void *dest, const void *src, uint32_t size);

void pmm_free(uint32_t address);
uint32_t pmm_alloc();

void pmm_init(uint32_t st_address, uint32_t size);

uint32_t pmm_free_frames(void);
uint32_t pmm_used_frames(void);