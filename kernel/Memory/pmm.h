#pragma once
#include <stdint.h>


#define MAX_FRAMES 32768

extern uint32_t mem[1024];


void pmm_free(uint32_t address);
uint32_t pmm_alloc();

void pmm_init(uint32_t st_address, uint32_t size);

uint32_t pmm_free_frames(void);
uint32_t pmm_get_top();

uint32_t pmm_used_frames(void);

uint32_t pmm_total_frames(void);