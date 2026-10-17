#include <stdio.h>
#include <string.h>
#include "../Networking/Foyer/foyer.h"

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

static uint8_t known_ip[4];
static uint8_t known_mac[6];
static int have_mac;

static int arp_requests;
static uint32_t next_pass = 1000, last_issued;

static int sent, sent_with_bad_pass, present_before_request;
static uint8_t sent_dst[6];
static uint32_t fake_now;

uint32_t get_ticks(void) { return fake_now; }
void kprintf(const char *f, ...) { (void)f; }

int rolodex_lookup(const uint8_t ip[4], uint8_t out_mac[6])
{
    if (have_mac && memcmp(ip, known_ip, 4) == 0)
    {
        memcpy(out_mac, known_mac, 6);
        return 1;
    }
    return 0;
}
int rolodex_dispatch_request(const uint8_t t[4], const uint8_t m[6], uint32_t *id)
{
    (void)t;
    (void)m;
    *id = next_pass++;

    last_issued = *id;

    arp_requests++;
    return 1;
}
const uint8_t *rolodex_last_request_frame(void)
{
    static uint8_t arp[42];
    return arp;
}
uint16_t rolodex_last_request_len(void) { return 42; }

static int pending_request;
int bailiff_request_pass(const uint8_t *frame, uint16_t len, uint32_t *id)
{
    (void)frame;
    (void)len;
    *id = next_pass++;
    last_issued = *id;
    pending_request = 1;
    return 1;
}
int bailiff_present_pass(uint32_t id, const uint8_t *frame, uint16_t len)
{
    (void)len;
    if (frame[12] != 0x08)
        return 1;
    if (!pending_request)
        present_before_request++;
    if (id != last_issued)
    {
        sent_with_bad_pass++;
        return 0;
    }
    sent++;
    memcpy(sent_dst, frame, 6);
    pending_request = 0;
    return 1;
}

static void reset(void)
{
    have_mac = 0;
    arp_requests = 0;
    sent = 0;
    sent_with_bad_pass = 0;
    present_before_request = 0;
    pending_request = 0;
    memset(sent_dst, 0xEE, 6);
}

int main(void)
{
    uint8_t our_mac[6] = {2, 0, 0, 0, 0, 1};
    uint8_t ip[4] = {10, 0, 2, 2};
    uint8_t mac[6] = {0x52, 0x55, 0x0a, 0x00, 0x02, 0x02};
    uint8_t frame[60];
    memcpy(known_ip, ip, 4);
    memcpy(known_mac, mac, 6);

    memset(frame, 0, sizeof(frame));
    frame[12] = 0x08;
    frame[13] = 0x00; // ethertype IPv4, destination MAC zeroed

    reset();
    CHECK(foyer_queue(ip, our_mac, frame, sizeof(frame)) == 1);
    CHECK(arp_requests == 1 && sent == 0);
    foyer_tick(our_mac);
    CHECK(sent == 0);

    have_mac = 1;
    foyer_tick(our_mac);
    CHECK(sent == 1 && memcmp(sent_dst, mac, 6) == 0);

    CHECK(sent_with_bad_pass == 0 && present_before_request == 0);

    reset();
    have_mac = 1;

    CHECK(foyer_queue(ip, our_mac, frame, sizeof(frame)) == 1);
    CHECK(sent == 1);

    CHECK(memcmp(sent_dst, mac, 6) == 0);

    CHECK(sent_with_bad_pass == 0 && present_before_request == 0);
    CHECK(arp_requests == 0);

    int ok = 1;
    for (int i = 0; i < 40; i++)
    {
        reset();
        have_mac = 1;

        if (foyer_queue(ip, our_mac, frame, sizeof(frame)) != 1 || sent != 1)
            ok = 0;
    }
    CHECK(ok);

    printf(fails ? "foyer: %d FAILED\n" : "foyer: all passed\n", fails);

    return fails != 0;
}