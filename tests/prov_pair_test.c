#include <stdio.h>
#include <string.h>
#include "../Networking/NetWatch/net_watch.h"
#include "../Networking/NetWatch/net_observe.h"
#include "../Networking/NetTrace/net_trace.h"
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

// 10.0.0.1:1234 -> 10.0.0.2:53
static const uint8_t request[] = {
    0x02, 0, 0, 0, 0, 2, 0x02, 0, 0, 0, 0, 1, 0x08, 0x00,
    0x45, 0, 0, 29, 0, 1, 0, 0, 64, 17, 0, 0, 10, 0, 0, 1, 10, 0, 0, 2,
    0x04, 0xD2, 0, 53, 0, 9, 0, 0, 'x'};

// 10.0.0.2:53 -> 10.0.0.1:1234
static const uint8_t reply[] = {
    0x02, 0, 0, 0, 0, 1, 0x02, 0, 0, 0, 0, 2, 0x08, 0x00,
    0x45, 0, 0, 29, 0, 1, 0, 0, 64, 17, 0, 0, 10, 0, 0, 2, 10, 0, 0, 1,
    0, 53, 0x04, 0xD2, 0, 9, 0, 0, 'y'};

// 10.0.0.2:99 -> 10.0.0.1:1234  (wrong port, must not pair)
static const uint8_t stray[] = {
    0x02, 0, 0, 0, 0, 1, 0x02, 0, 0, 0, 0, 2, 0x08, 0x00,
    0x45, 0, 0, 29, 0, 1, 0, 0, 64, 17, 0, 0, 10, 0, 0, 2, 10, 0, 0, 1,
    0, 99, 0x04, 0xD2, 0, 9, 0, 0, 'z'};

static provenance_entity_ref_t N(uint32_t id)
{
    provenance_entity_ref_t r = {PROVENANCE_ENTITY_NETWORK, id};
    return r;
}

static uint32_t triggered_edges(void)
{
    static provenance_snapshot_t snap;

    uint32_t n = 0;

    provenance_get_snapshot(&snap);

    for (uint32_t i = 0; i < snap.record_count; i++)
        if (snap.records[i].relation == PROVENANCE_RELATION_TRIGGERED)
            n++;
    return n;
}

int main(void)
{
    static netwatch_snapshot_t ns;
    provenance_record_t res[16];

    provenance_init();
    netwatch_init();

    netobserve_init();
    nettrace_init();

    netobserve_capture(NETWATCH_TX, request, sizeof(request));

    netobserve_capture(NETWATCH_RX, reply, sizeof(reply));
    netobserve_capture(NETWATCH_RX, stray, sizeof(stray));

    netobserve_drain();
    CHECK(netwatch_get_event_count() == 3);
    CHECK(triggered_edges() == 0); // nothing paired until nettrace runs

    netwatch_get_snapshot(&ns);
    nettrace_rebuild(&ns);
    CHECK(triggered_edges() == 1);

    uint32_t req_id = ns.events[0].id, rep_id = ns.events[1].id, stray_id = ns.events[2].id;
    CHECK(provenance_trace_from(N(req_id), res, 16) == 1);
    CHECK(res[0].target.id == rep_id && res[0].relation == PROVENANCE_RELATION_TRIGGERED && res[0].reason == PROVENANCE_REASON_RESPONSE);

    for (int i = 0; i < 5; i++)
        nettrace_rebuild(&ns);
    CHECK(triggered_edges() == 1);

    CHECK(provenance_trace_to(N(rep_id), res, 16) == 3);

    // stray packet never paired
    CHECK(provenance_trace_to(N(stray_id), res, 16) == 1);

    // record_once keeps first record's tick and id
    uint32_t before = provenance_count();
    CHECK(provenance_record_once(N(req_id), N(rep_id), PROVENANCE_RELATION_TRIGGERED, PROVENANCE_REASON_RESPONSE) != 0);
    CHECK(provenance_count() == before);

    printf(fails ? "pairing: %d FAILED\n" : "pairing: all passed\n", fails);
    return fails != 0;
}