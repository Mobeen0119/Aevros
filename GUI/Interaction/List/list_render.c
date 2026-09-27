#include "list_render.h"

#include "list.h"

#include "../Window/window.h"
#include "../../Algo/Font/font.h"

#include "../../Algo/Render/render.h"

#define LIST_RENDER_TEXT_PAD_Y 4
#define LIST_RENDER_TEXT_PAD_X 4

void list_render_all(void)
{
    int32_t origin_x, origin_y;

    uint32_t row_width, row_height;

    if (!list_get_layout(&origin_x, &origin_y, &row_width, &row_height))
        return;

    list_entry_t entries[LIST_MAX_ENTRIES];
    uint32_t n = list_get_entries(entries, LIST_MAX_ENTRIES);

    for (uint32_t i = 0; i < n; i++)
    {
        int32_t row_y = origin_y + (int32_t)(i * row_height);
        uint32_t color = render_color_for_state(entries[i].state);

        font_draw_string(origin_x + LIST_RENDER_TEXT_PAD_X,
                         row_y + LIST_RENDER_TEXT_PAD_Y,
                         entries[i].name, color);
    }
}

const char *list_render_dependency_note(void)
{
    return "List keeps computing entries and layout fine if this stops "
           "nothing crashes. The list just stops being visible; clicking it "
           "(list_at_point) still works, so the launcher would keep "
           "functioning blind, which is worse than it sounds.";
}

bool list_render_selftest(void)
{
    registry_init();
    window_init_system();

    list_render_all();

    list_init(0, 0, 200, 20);
    list_render_all();

    registry_register("terminal");

    window_init(1, 10, 10, 50, 50, "terminal");
    registry_register("orphan");

    list_render_all();

    return true;
}