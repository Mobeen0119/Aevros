#include <stdio.h>
#include <string.h>
#include "../Networking/NetWatch/net_watch.h"
#include "../Networking/NetWatch/net_observe.h"
#include "../Provenance/provenance.h"

static const uint8_t udp_frame[] = {
    0x02, 0, 0, 0, 0, 2, 0x02, 0, 0, 0, 0, 1, 0x08, 0x00,
    0x45, 0, 0, 29, 0, 1, 0, 0, 64, 17, 0, 0, 10, 0, 0, 1, 10, 0, 0, 2,
    0x04, 0xD2, 0, 53, 0, 9, 0, 0, 'x'};

static const uint8_t arp_frame[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x02, 0, 0, 0, 0, 1, 0x08, 0x06,
    0, 1, 0x08, 0, 6, 4, 0, 1,
    0x02, 0, 0, 0, 0, 1, 10, 0, 0, 1,
    0, 0, 0, 0, 0, 0, 10, 0, 0, 2};

static const char *ENT[] = {"NONE", "INTERRUPT", "DRIVER", "PROCESS", "MEMORY", "NETWORK", "FILE", "DEVICE", "OPERATION"};
static const char *REL[] = {"NONE", "CAUSED", "AFFECTED", "CREATED", "DESTROYED", "OWNED", "TRIGGERED", "DEPENDED_ON", "RESPONDED_TO"};
static const char *WHY[] = {"NONE", "INTERRUPT", "SCHEDULE", "ALLOCATION", "DEALLOCATION", "CREATION", "DESTRUCTION", "REQUEST", "RESPONSE", "DELIVERY", "FAILURE", "DEPENDENCY", "USER_ACTION", "SYSTEM_ACTION"};

static void step(const char *title)
{
    printf("\n=== %s ===\n", title);
}

static void show_edges(const char *label, const provenance_record_t *r, uint32_t n)
{
    printf("%s: %u edge(s)\n", label, n);

    for (uint32_t i = 0; i < n; i++)
        printf("   [%u] %s:%u --%s--> %s:%u (%s)\n", r[i].id, ENT[r[i].source.type], r[i].source.id,
               REL[r[i].relation], ENT[r[i].target.type], r[i].target.id, WHY[r[i].reason]);
}

int main(void)
{
    provenance_record_t res[32];

    step("1. boot: provenance_init, netwatch_init, netobserve_init");
    provenance_init();
    netwatch_init();
    netobserve_init();

    step("2. shell> provenance   (nothing has happened yet)");
    provenance_report();

    step("3. NIC sees traffic: 1 UDP in, 1 UDP out, 1 ARP in (queued, not parsed yet)");
    netobserve_capture(NETWATCH_RX, udp_frame, sizeof(udp_frame));

    netobserve_capture(NETWATCH_TX, udp_frame, sizeof(udp_frame));
    netobserve_capture(NETWATCH_RX, arp_frame, sizeof(arp_frame));
    printf("provenance records so far: %u (the timer tick has not drained the queue)\n", provenance_count());

    step("4. timer tick runs netobserve_drain()");
    netobserve_drain();
    printf("netwatch events: %u, provenance records: %u\n", netwatch_get_event_count(), provenance_count());

    step("5. shell> provenance");
    provenance_report();

    step("6. trace: where did the first received packet come from?");
    const netwatch_event_t *e = netwatch_get_event(0);
    provenance_entity_ref_t pkt = {PROVENANCE_ENTITY_NETWORK, e ? e->id : 0};
    show_edges("trace_to(NETWORK:first rx)", res, provenance_trace_to(pkt, res, 32));

    step("7. trace: everything the NIC driver produced, oldest first");
    provenance_entity_ref_t nic = {PROVENANCE_ENTITY_DRIVER, 1};
    show_edges("trace_from(DRIVER:1)", res, provenance_trace_from(nic, res, 32));

    step("8. flood: 300 more frames, table holds 256");
    for (int i = 0; i < 300; i++)
    {
        netobserve_capture(NETWATCH_RX, udp_frame, sizeof(udp_frame));
        netobserve_drain();
    }
    printf("records kept: %u, overwritten: %u\n", provenance_count(), provenance_dropped());
    printf("oldest kept id: ");
    static provenance_snapshot_t snap;
    provenance_get_snapshot(&snap);
    printf("%u, newest id: %u\n", snap.records[0].id, snap.records[snap.record_count - 1].id);

    printf("\ndemo finished\n");
    return 0;
}