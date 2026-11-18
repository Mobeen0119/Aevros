#ifndef STORE_H
#define STORE_H
#include <stdint.h>

int store_save(const char *name, const uint8_t *data, uint32_t size);

int store_load(const char *name, uint8_t *out, uint32_t max, uint32_t *size_out);

void store_list(void);

#endif