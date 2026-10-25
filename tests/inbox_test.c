#include <stdio.h>
#include <string.h>
#include "../Networking/IPv4/Inbox/inbox.h"
#include "../Networking/IPv4/LockBox/lockbox.h"

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

void kprintf(const char *f, ...) { (void)f; }
uint32_t get_ticks(void) { return 0; }

int main(void)
{
    uint8_t peer[4] = {10, 0, 2, 2};
    uint8_t data[2000], out[5000];
    uint32_t id;

    memset(data, 0x5A, sizeof(data));

    // claim a connection slot the way an accepted connection does
    lockbox_listen(7007, 6, &id);
    lockbox_result_t r = lockbox_claim(7007, peer, 40000, 6, &id);
    CHECK(r == LOCKBOX_OK);

    // fill to byte: 1440 + 1440 + 1216 = 4096
    CHECK(inbox_deposit(id, data, 1440) == 1);
    CHECK(inbox_deposit(id, data, 1440) == 1);
    CHECK(inbox_deposit(id, data, 1216) == 1);
    CHECK(inbox_available(id) == 4096);

    CHECK(inbox_deposit(id, data, 1) == 0);

    // everything comes back out in order, space is usable again
    CHECK(inbox_read(id, out, sizeof(out)) == 4096);
    CHECK(inbox_available(id) == 0);
    CHECK(inbox_deposit(id, data, 2000) == 1);
    CHECK(inbox_available(id) == 2000);

    printf(fails ? "inbox: %d FAILED\n" : "inbox: all passed\n", fails);
    return fails != 0;
}