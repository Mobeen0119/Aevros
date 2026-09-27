#ifndef LIST_RENDER_H
#define LIST_RENDER_H

#include <stdbool.h>

void list_render_all(void);

const char *list_render_dependency_note(void);

bool list_render_selftest(void);

#endif