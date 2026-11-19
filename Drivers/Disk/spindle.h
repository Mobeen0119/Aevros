#ifndef SPINDLE_H
#define SPINDLE_H
#include <stdint.h>

int spindle_init(void);

uint32_t spindle_sector_count(void);

int spindle_read(uint32_t lba, uint32_t count, uint8_t *buf);

int spindle_write(uint32_t lba, uint32_t count, const uint8_t *buf);

#endif