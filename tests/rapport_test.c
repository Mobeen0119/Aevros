#include <stdio.h>
#include "../Networking/IPv4/Conversation/rapport.h"

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
void lockbox_release(uint32_t id) { (void)id; }
void scheduler_cancel(uint32_t id) { (void)id; }
uint32_t scheduler_bytes_in_flight(uint32_t id)
{
    (void)id;
    return 0;
}

int main(void)
{
    // connection accepted from a peer whose ISN is 1000... ours is 5000
    rapport_on_syn(5, 1000, 5000);
    CHECK(rapport_get_state(5) == CONV_SYN_RECEIVED);
    CHECK(rapport_get_send_seq(5) == 5001);
    CHECK(rapport_get_expected_seq(5) == 1001);

    // ACK that does not cover our SYN-ACK must not open the connection
    rapport_on_ack(5, 4999);
    CHECK(rapport_get_state(5) == CONV_SYN_RECEIVED);
    rapport_on_ack(5, 5000);
    CHECK(rapport_get_state(5) == CONV_SYN_RECEIVED);
    rapport_on_ack(5, 5001);
    CHECK(rapport_get_state(5) == CONV_ESTABLISHED);

    rapport_advance_send_seq(5, 13);
    CHECK(rapport_get_send_seq(5) == 5014);

    //  slot reused by a new connection starts from its own sequence number
    rapport_on_syn(5, 70000, 90000);
    CHECK(rapport_get_send_seq(5) == 90001);

    rapport_on_syn(6, 200, 7777);
    rapport_on_ack(6, 7778);
    CHECK(rapport_get_state(6) == CONV_ESTABLISHED);
    CHECK(rapport_get_send_seq(6) == 7778);

    printf(fails ? "rapport: %d FAILED\n" : "rapport: all passed\n", fails);
    return fails != 0;
}