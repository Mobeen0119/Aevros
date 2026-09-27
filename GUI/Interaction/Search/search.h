#ifndef SEARCH_H
#define SEARCH_H

#include <stdint.h>
#include <stdbool.h>
#include "../List/list.h"

#define SEARCH_MAX_QUERY_LEN 32

void search_set_query(const char *query);

void search_clear_query(void);
uint32_t search_get_query(char *out, uint32_t max_len);

uint32_t search_filter(const list_entry_t *in, uint32_t in_count, list_entry_t *out, uint32_t max_out);

const char *search_dependency_note(void);

bool search_selftest(void);

#endif