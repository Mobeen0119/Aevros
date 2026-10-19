#include <stdio.h>
#include <string.h>
#include "../Networking/Scheduler/scheduler.h"

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

void rapport_on_rst(uint32_t c) { (void)c; }

static int resent;
int bailiff_request_pass(const uint8_t *f, uint16_t l, uint32_t *id)
{
    (void)f;
    (void)l;

    *id = 1;
    return 1;
}
int bailiff_present_pass(uint32_t id, const uint8_t *f, uint16_t l)
{
    (void)id;
    (void)f;
    (void)l;

    resent++;
    return 1;
}

int main(void)
{
    uint8_t frame[80];
    memset(frame, 0, sizeof(frame));

    // SYN-ACK: a 58-byte frame uses ONE sequence number.... peer ACKs isn + 1
    CHECK(scheduler_track(3, 1000, 1, frame, 58) == 1);
    CHECK(scheduler_bytes_in_flight(3) == 1);
    scheduler_ack(3, 1000);

    CHECK(scheduler_bytes_in_flight(3) == 1);

    scheduler_ack(3, 1001);
    CHECK(scheduler_bytes_in_flight(3) == 0);

    // 13 bytes of data in a 67-byte frame... acked only when  peer has all 13
    CHECK(scheduler_track(3, 5000, 13, frame, 67) == 1);
    CHECK(scheduler_bytes_in_flight(3) == 13);
    scheduler_ack(3, 5006); // partial ack
    CHECK(scheduler_bytes_in_flight(3) == 13);
    scheduler_ack(3, 5013);
    CHECK(scheduler_bytes_in_flight(3) == 0);

    // FIN uses one sequence number
    CHECK(scheduler_track(4, 9000, 1, frame, 54) == 1);
    scheduler_ack(4, 9001);
    CHECK(scheduler_bytes_in_flight(4) == 0);

    // unacked segment is resent (with the full frame)... and clears once finally acked
    resent = 0;
    CHECK(scheduler_track(5, 100, 1, frame, 58) == 1);
    for (int i = 0; i < 60; i++)
    {
        now += 50;
        scheduler_tick();
    }
    CHECK(resent >= 1);
    scheduler_ack(5, 101);
    CHECK(scheduler_bytes_in_flight(5) == 0);

    printf(fails ? "scheduler: %d FAILED\n" : "scheduler: all passed\n", fails);
    return fails != 0;
}