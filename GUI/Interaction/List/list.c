#include "list.h"
#include "../Window/window.h"

#include "../../../Lib/string.h"

static int32_t list_origin_x = 0;
static int32_t list_origin_y = 0;

static uint32_t list_row_width = 0;
static uint32_t list_row_height = 0;

void list_init(int32_t origin_x, int32_t origin_y, uint32_t row_width, uint32_t row_height)
{
    list_origin_x = origin_x;
    list_origin_y = origin_y;

    list_row_width = row_width;
    list_row_height = row_height;
}

uint32_t list_get_entries(list_entry_t *out, uint32_t max_entries)
{
    registry_entry_t reg[LIST_MAX_ENTRIES];
    uint32_t reg_count = registry_list(reg, LIST_MAX_ENTRIES);

    window_t wins[WINDOW_MAX_WINDOWS];

    uint32_t win_count = window_list(wins, WINDOW_MAX_WINDOWS);

    uint32_t n = 0;
    for (uint32_t i = 0; i < reg_count && n < max_entries; i++)
    {
        strcpy(out[n].name, reg[i].name);

        out[n].state = reg[i].state;
        out[n].has_window = false;
        out[n].wid = 0;

        for (uint32_t j = 0; j < win_count; j++)
        {
            if (strcmp(wins[j].owner, reg[i].name) == 0)
            {
                out[n].wid = wins[j].wid;
                out[n].has_window = true;
                break;
            }
        }

        n++;
    }
    return n;
}

bool list_at_point(int32_t x, int32_t y, char *out_name)
{
    if (list_row_height == 0)
        return false;

    if (x < list_origin_x || x >= list_origin_x + (int32_t)list_row_width)
        return false;
    if (y < list_origin_y)
        return false;

    uint32_t row = (uint32_t)(y - list_origin_y) / list_row_height;

    list_entry_t entries[LIST_MAX_ENTRIES];
    uint32_t count = list_get_entries(entries, LIST_MAX_ENTRIES);

    if (row >= count)
        return false;

    strcpy(out_name, entries[row].name);
    return true;
}

const char *list_dependency_note(void)
{
    return "Registry and Window keep working independently if List stops "
           "list_get_entries() and list_at_point() just stop being callable, "
           "which means nothing can discover apps to launch/focus by name "
           "anymore. Nothing crashes; the launcher surface goes blind.";
}

bool list_selftest(void)
{
    registry_init();
    window_init_system();

    registry_register("terminal");
    window_init(1, 10, 10, 50, 50, "terminal");
    registry_register("files");
    window_init(2, 200, 10, 50, 50, "files");
    registry_register("orphan");

    list_entry_t entries[LIST_MAX_ENTRIES];
    uint32_t n = list_get_entries(entries, LIST_MAX_ENTRIES);
    if (n != 3)
        return false;

    if (strcmp(entries[0].name, "terminal") != 0)
        return false;
    if (strcmp(entries[1].name, "files") != 0)
        return false;
    if (strcmp(entries[2].name, "orphan") != 0)
        return false;

    if (!entries[0].has_window || entries[0].wid != 1)
        return false;
    if (!entries[1].has_window || entries[1].wid != 2)
        return false;
    if (entries[2].has_window)
        return false;

    if (entries[0].state != ENTRY_NOT_RUNNING)
        return false;
    registry_mark_launched("terminal");
    n = list_get_entries(entries, LIST_MAX_ENTRIES);
    if (entries[0].state != ENTRY_RUNNING_UNVERIFIED)
        return false;

    list_init(0, 100, 120, 50);

    char name[REGISTRY_NAME_LEN];
    if (!list_at_point(10, 100, name) || strcmp(name, "terminal") != 0)
        return false;
    if (!list_at_point(10, 149, name) || strcmp(name, "terminal") != 0)
        return false;
    if (!list_at_point(10, 150, name) || strcmp(name, "files") != 0)
        return false;
    if (!list_at_point(10, 249, name) || strcmp(name, "orphan") != 0)
        return false;

    if (list_at_point(10, 250, name))
        return false;

    if (list_at_point(-1, 120, name))
        return false;
    if (list_at_point(200, 120, name))
        return false;
    if (list_at_point(10, 50, name))
        return false;

    if (!list_at_point(10, 249, name) || strcmp(name, "orphan") != 0)
        return false;

    return true;
}