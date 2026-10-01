#ifndef LIST_RENDER_H
#define LIST_RENDER_H

#include <stdbool.h>
#include "../../Core/Panel/panel.h"

#define LIST_PANEL_X 8
#define LIST_PANEL_Y 8
#define LIST_PANEL_W 200
#define LIST_ROW_H 16
#define LIST_MAX_VISIBLE 12

void list_render_all(void);

panel_rect_t list_panel_rect(void);

const char *list_render_dependency_note(void);

bool list_render_selftest(void);

#endif