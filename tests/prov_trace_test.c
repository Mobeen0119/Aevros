#include <stdio.h>
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

static provenance_entity_ref_t E(provenance_entity_t t, uint32_t id)
{
    provenance_entity_ref_t r = {t, id};
    return r;
}

int main(void)
{
    provenance_record_t res[64];
    provenance_entity_ref_t irq = E(PROVENANCE_ENTITY_INTERRUPT, 11);

    provenance_entity_ref_t nic = E(PROVENANCE_ENTITY_DRIVER, 1);
    provenance_entity_ref_t pkt = E(PROVENANCE_ENTITY_NETWORK, 7);

    provenance_entity_ref_t proc = E(PROVENANCE_ENTITY_PROCESS, 3);

    provenance_init();
    CHECK(provenance_record(irq, nic, PROVENANCE_RELATION_TRIGGERED, PROVENANCE_REASON_INTERRUPT) == 1);
    CHECK(provenance_record(nic, pkt, PROVENANCE_RELATION_CREATED, PROVENANCE_REASON_DELIVERY) == 2);

    CHECK(provenance_record(pkt, proc, PROVENANCE_RELATION_AFFECTED, PROVENANCE_REASON_RESPONSE) == 3);
    CHECK(provenance_record(proc, irq, PROVENANCE_RELATION_CAUSED, PROVENANCE_REASON_SYSTEM_ACTION) == 4); // cycle

    uint32_t n = provenance_trace_from(irq, res, 64);
    CHECK(n == 4 && res[0].id == 1 && res[1].id == 2 && res[2].id == 3 && res[3].id == 4);

    n = provenance_trace_to(irq, res, 64);
    CHECK(n == 4 && res[0].id == 4 && res[1].id == 3 && res[2].id == 2 && res[3].id == 1);

    CHECK(provenance_trace_from(irq, res, 2) == 2);
    CHECK(provenance_trace_from(irq, res, 0) == 0);
    CHECK(provenance_trace_from(irq, 0, 5) == 0);

    provenance_init();

    for (uint32_t i = 0; i < 300; i++)
        provenance_record(irq, nic, PROVENANCE_RELATION_CAUSED, PROVENANCE_REASON_INTERRUPT);

    static provenance_record_t big[300];
    n = provenance_trace_from(irq, big, 300);
    CHECK(n == 256 && big[0].id == 45 && big[255].id == 300);

    printf(fails ? "trace: %d FAILED\n" : "trace: all passed\n", fails);
    return fails != 0;
}