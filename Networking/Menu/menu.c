#include "menu.h"

typedef struct
{
    uint16_t port;
    uint8_t protocol;
    int in_use;
} menu_entry_t;

static menu_entry_t entries[MENU_CAPACITY];

static menu_entry_t *menu_find_entry(uint16_t port, uint8_t protocol)
{
    for (int i = 0; i < MENU_CAPACITY; i++)
        if (entries[i].in_use && entries[i].protocol == protocol && entries[i].port == port)
            return &entries[i];

    return 0;
}

int menu_open_port(uint16_t port, uint8_t protocol)
{
    if (menu_find_entry(port, protocol))
        return 1;

    for (int i = 0; i < MENU_CAPACITY; i++)
    {
        if (!entries[i].in_use)
        {
            entries[i].port = port;
            entries[i].protocol = protocol;
            entries[i].in_use = 1;

            return 1;
        }
    }
    return 0;
}

void menu_close_port(uint16_t port, uint8_t protocol)
{
    menu_entry_t *e = menu_find_entry(port, protocol);
    if (e)
        e->in_use = 0;
}

int menu_is_open(uint16_t port, uint8_t protocol)
{
    return menu_find_entry(port, protocol) != 0;
}

uint32_t menu_count(void)
{
    uint32_t n = 0;

    for (int i = 0; i < MENU_CAPACITY; i++)
        if (entries[i].in_use)
            n++;

        return n;
}