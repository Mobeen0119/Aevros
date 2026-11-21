#ifndef SHELF_H
#define SHELF_H
#include <stdint.h>

int shelf_save(const char *name, uint8_t *data, uint32_t size, uint32_t capacity);

int shelf_load(const char *name, uint8_t *out, uint32_t capacity, uint32_t *size_out);

void shelf_list(void);

#endif