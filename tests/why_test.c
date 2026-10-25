#include <stdio.h>
#include <string.h>
#include "../Networking/Why/why.h"

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
void kprintf(const char *f, ...) { (void)f; }

int main(void)
{
    why_init();
    CHECK(why_top_reason(3) == WHY_NONE);

    now = 1000;
    CHECK(why_note(3, WHY_SEND_WAITING_ACK) == 1);
    int spoken = 0;
    for (int i = 0; i < 5000; i++)
    {
        now = 1000 + i;
        spoken += why_note(3, WHY_SEND_WAITING_ACK);
    }
    CHECK(why_count(3, WHY_SEND_WAITING_ACK) == 5001);
    CHECK(spoken <= 52); // 5000 calls over 50 seconds
    CHECK(spoken >= 45);

    now = 2000;
    CHECK(why_note(4, WHY_SEND_WAITING_ACK) == 1);
    CHECK(why_note(3, WHY_OUT_OF_ORDER) == 1);
    CHECK(why_count(4, WHY_SEND_WAITING_ACK) == 1);
    CHECK(why_count(3, WHY_OUT_OF_ORDER) == 1);

    CHECK(why_top_reason(3) == WHY_SEND_WAITING_ACK);
    for (int i = 0; i < 6000; i++)
    {
        now += 1;
        why_note(3, WHY_RETRANSMIT_TIMEOUT);
    }
    CHECK(why_top_reason(3) == WHY_RETRANSMIT_TIMEOUT);
    CHECK(strlen(why_reason_text(WHY_RETRANSMIT_TIMEOUT)) > 10);

    // bad input is ignored
    CHECK(why_note(9999, WHY_SEND_WAITING_ACK) == 0);
    CHECK(why_note(3, WHY_NONE) == 0);
    CHECK(why_count(9999, WHY_OUT_OF_ORDER) == 0);

    why_init();
    CHECK(why_count(3, WHY_SEND_WAITING_ACK) == 0 && why_top_reason(3) == WHY_NONE);

    printf(fails ? "why: %d FAILED\n" : "why: all passed\n", fails);
    return fails != 0;
}