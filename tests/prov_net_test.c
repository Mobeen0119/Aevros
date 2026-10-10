#include <stdio.h>
#include <string.h>
#include "../Networking/NetWatch/net_watch.h"
#include "../Networking/NetWatch/net_observe.h"
#include "../Provenance/provenance.h"

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

static const uint8_t udp_frame[] = {
    0x02, 0, 0, 0, 0, 2, 0x02, 0, 0, 0, 0, 1, 0x08, 0x00,
    0x45, 0, 0, 29, 0, 1, 0, 0, 64, 17, 0, 0, 10, 0, 0, 1, 10, 0, 0, 2,
    0x04, 0xD2, 0, 53, 0, 9, 0, 0, 'x'};

int main(void)
{
    provenance_record_t res[16];
    provenance_entity_ref_t nic = {PROVENANCE_ENTITY_DRIVER, 1};

    provenance_init();
    netwatch_init();
    netobserve_init();

    netobserve_capture(NETWATCH_RX, udp_frame, sizeof(udp_frame));
    netobserve_capture(NETWATCH_TX, udp_frame, sizeof(udp_frame));
    CHECK(provenance_count() == 0);
    netobserve_drain();

    CHECK(netwatch_get_event_count() == 2);
    CHECK(provenance_count() == 2);

    const netwatch_event_t *rx = netwatch_get_event(0);
    const netwatch_event_t *tx = netwatch_get_event(1);
    CHECK(rx && rx->direction == NETWATCH_RX && tx && tx->direction == NETWATCH_TX);

    provenance_entity_ref_t rx_pkt = {PROVENANCE_ENTITY_NETWORK, rx ? rx->id : 0};
    provenance_entity_ref_t tx_pkt = {PROVENANCE_ENTITY_NETWORK, tx ? tx->id : 0};

    CHECK(provenance_trace_to(rx_pkt, res, 16) == 1);
    CHECK(res[0].source.type == PROVENANCE_ENTITY_DRIVER && res[0].relation == PROVENANCE_RELATION_CREATED);
    CHECK(res[0].reason == PROVENANCE_REASON_DELIVERY);

    CHECK(provenance_trace_to(tx_pkt, res, 16) == 1);
    CHECK(res[0].source.type == PROVENANCE_ENTITY_DRIVER && res[0].reason == PROVENANCE_REASON_SYSTEM_ACTION);

    CHECK(provenance_trace_from(nic, res, 16) == 2);
    CHECK(res[0].target.id == rx_pkt.id && res[1].target.id == tx_pkt.id);

    CHECK(provenance_find_relation(nic, PROVENANCE_RELATION_CREATED, res, 16) == 2);

    printf(fails ? "net->provenance: %d FAILED\n" : "net->provenance: all passed\n", fails);
    return fails != 0;
}