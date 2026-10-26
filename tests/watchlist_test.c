#include <stdio.h>
#include <string.h>
#include "../Networking/WatchList/watchlist.h"

static int fails;
#define CHECK(c)                                        \
    do                                                  \
    {                                                   \
        if (c)                                          \
            printf("PASS: %s\n", #c);                   \
        else                                            \
        {                                               \
            printf("FAIL line %d: %s\n", __LINE__, #c); \
            fails++;                                    \
        }                                               \
    } while (0)

static uint32_t now;
uint32_t get_ticks(void) { return now; }
void ids_notify(int event, int a, const char *msg)
{
    (void)event;
    (void)a;
    (void)msg;
}

int main(void)
{
    uint8_t gateway[6] = {0x52, 0x55, 0x0a, 0x00, 0x02, 0x02};
    uint8_t other[6] = {0x02, 0, 0, 0, 0, 9};
    int dropped = 0;

    for (uint32_t t = 0; t < 1000; t++)
    {
        now = 1000 + t;
        for (int k = 0; k < 8; k++)
            if (watchlist_is_flagged(gateway))
                dropped++;
    }
    CHECK(dropped == 0);
    CHECK(watchlist_flagged_count() == 0);

    int flagged_at = -1;
    for (int i = 0; i < 20000; i++)
    {
        now = 5000 + i / 200;
        if (watchlist_strike(other) && flagged_at < 0)
            flagged_at = i;
    }
    CHECK(flagged_at == WATCH_THRESHOLD - 1);
    CHECK(watchlist_flagged_count() == 1);

    for (int i = 0; i < 500; i++)
        if (watchlist_is_flagged(other))
            watchlist_note_dropped();
    CHECK(watchlist_dropped_count() == 500);

    now = 5000 + 100 + 300;
    CHECK(watchlist_is_flagged(other) == 0);
    CHECK(watchlist_flagged_count() == 0);

    now += 1;
    CHECK(watchlist_is_flagged(gateway) == 0);

    printf(fails ? "watchlist: %d FAILED\n" : "watchlist: all passed\n", fails);
    return fails != 0;
}