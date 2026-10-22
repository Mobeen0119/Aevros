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
    uint8_t frame[128];
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

    CHECK(scheduler_free_slots(6) == SCHED_WINDOW);
    for (int i = 0; i < SCHED_WINDOW; i++)
        CHECK(scheduler_track(6, 2000 + i * 100, 100, frame, 70) == 1);
    CHECK(scheduler_free_slots(6) == 0);
    CHECK(scheduler_bytes_in_flight(6) == 4 * 100);
    CHECK(scheduler_track(6, 2400, 100, frame, 70) == 0);

    // one cumulative ACK frees every segment it fully covers, and only those
    scheduler_ack(6, 2250); // covers the segments ending at 2100 and 2200
    CHECK(scheduler_free_slots(6) == 2);
    CHECK(scheduler_bytes_in_flight(6) == 200);
    scheduler_ack(6, 2400);
    CHECK(scheduler_free_slots(6) == SCHED_WINDOW);
    CHECK(scheduler_bytes_in_flight(6) == 0);

    // a timeout resends only the OLDEST unacknowledged segment
    resent = 0;
    CHECK(scheduler_track(7, 3000, 100, frame, 70) == 1);
    CHECK(scheduler_track(7, 3100, 100, frame, 70) == 1);
    now += 500;
    scheduler_tick();
    CHECK(resent == 1);
    scheduler_ack(7, 3200);
    CHECK(scheduler_bytes_in_flight(7) == 0);

    CHECK(scheduler_track(8, 0xFFFFFFF0u, 32, frame, 90) == 1);
    scheduler_ack(8, 0xFFFFFFF8u); // not yet covered
    CHECK(scheduler_bytes_in_flight(8) == 32);
    scheduler_ack(8, 16); // 0xFFFFFFF0 + 32 wraps to 16
    CHECK(scheduler_bytes_in_flight(8) == 0);

    // cancel clears the whole window
    CHECK(scheduler_track(9, 10, 5, frame, 60) == 1 && scheduler_track(9, 15, 5, frame, 60) == 1);
    scheduler_cancel(9);
    CHECK(scheduler_free_slots(9) == SCHED_WINDOW && scheduler_bytes_in_flight(9) == 0);

    printf(fails ? "scheduler: %d FAILED\n" : "scheduler: all passed\n", fails);
    return fails != 0;
}