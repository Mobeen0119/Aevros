#include <stdio.h>
#include <string.h>
#include "../Networking/IPv4/Postbox/postbox.h"

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

int main(void)
{
    static uint8_t buf[600], out[600];

    uint8_t src[4] = {10, 0, 2, 2}, got_ip[4];
    uint16_t got_port;

    // DHCP offer from QEMU carries 548 bytes of UDP payload clients must accept 548
    memset(buf, 0xAB, sizeof(buf));

    CHECK(postbox_deposit(3, src, 67, buf, 548) == 1);
    CHECK(postbox_read(3, got_ip, &got_port, out, sizeof(out)) == 548);

    CHECK(got_port == 67 && got_ip[3] == 2 && out[547] == 0xAB);

    // anything over the limit refused
    CHECK(postbox_deposit(3, src, 67, buf, POSTBOX_MAX_DATAGRAM + 1) == 0);

    // the queue must wrap....40 deposit/read cycles through 8-deep queue, data must come back intact
    int ok = 1;
    for (int i = 0; i < 40; i++)
    {
        memset(buf, i, 100);
        if (!postbox_deposit(5, src, (uint16_t)(1000 + i), buf, 100))
        {
            ok = 0;
            break;
        }
        if (postbox_read(5, got_ip, &got_port, out, sizeof(out)) != 100)
        {
            ok = 0;
            break;
        }
        if (got_port != 1000 + i || out[0] != (uint8_t)i || out[99] != (uint8_t)i)
        {
            ok = 0;
            break;
        }
    }
    CHECK(ok);

    // 8 pending fills it... the ninth is refused
    for (int i = 0; i < 8; i++)
    {
        memset(buf, 0x40 + i, 10);
        CHECK(postbox_deposit(7, src, (uint16_t)i, buf, 10) == 1);
    }

    CHECK(postbox_deposit(7, src, 99, buf, 10) == 0);
    int order = 1;

    for (int i = 0; i < 8; i++)
    {
        postbox_read(7, got_ip, &got_port, out, sizeof(out));

        if (got_port != i || out[0] != 0x40 + i)
            order = 0;
    }
    CHECK(order);
    CHECK(postbox_pending_count(7) == 0);

    printf(fails ? "postbox: %d FAILED\n" : "postbox: all passed\n", fails);
    return fails != 0;
}