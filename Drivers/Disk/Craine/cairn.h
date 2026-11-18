#ifndef CAIRN_H
#define CAIRN_H
#include <stdint.h>

int cairn_save(const char *name, const uint8_t *data, uint32_t size);

int cairn_load(const char *name, uint8_t *out, uint32_t max, uint32_t *size_out);

void cairn_list(void);

#endif