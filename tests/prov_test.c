#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "../Provenance/provenance.h"

int main(void)
{
    provenance_init();

    provenance_entity_ref_t process = {PROVENANCE_ENTITY_PROCESS, 7};
    provenance_entity_ref_t file = {PROVENANCE_ENTITY_FILE, 12};
    provenance_entity_ref_t device = {PROVENANCE_ENTITY_DEVICE, 3};

    provenance_record_t results[PROVENANCE_MAX_RECORDS + 1];
    provenance_record_t record;

    uint32_t first = provenance_record(process, file, PROVENANCE_RELATION_CREATED, PROVENANCE_REASON_CREATION);

    uint32_t second = provenance_record(file, device, PROVENANCE_RELATION_DEPENDED_ON, PROVENANCE_REASON_DEPENDENCY);

    assert(first != 0);
    assert(second != 0);
    assert(provenance_count() == 2);
    printf("PASS: record creation\n");

    assert(provenance_get(first, &record));
    assert(record.source.type == PROVENANCE_ENTITY_PROCESS);
    assert(record.source.id == 7);

    assert(record.target.type == PROVENANCE_ENTITY_FILE);
    assert(record.target.id == 12);
    printf("PASS: record lookup\n");

    assert(!provenance_get(0xFFFFFFFFu, &record));
    printf("PASS: nonexistent record lookup\n");

    uint32_t forward = provenance_trace_from(process, results, PROVENANCE_MAX_RECORDS);
    assert(forward == 2);
    printf("PASS: forward traversal (%u records)\n", forward);

    uint32_t backward = provenance_trace_to(device, results, PROVENANCE_MAX_RECORDS);
    assert(backward == 2);
    printf("PASS: reverse traversal (%u records)\n", backward);

    provenance_record(device, process, PROVENANCE_RELATION_RESPONDED_TO, PROVENANCE_REASON_RESPONSE);

    forward = provenance_trace_from(process, results, PROVENANCE_MAX_RECORDS);
    assert(forward == 3);

    printf("PASS: cyclic forward traversal (%u records)\n", forward);

    backward = provenance_trace_to(device, results, PROVENANCE_MAX_RECORDS);
    assert(backward == 3);
    printf("PASS: cyclic reverse traversal (%u records)\n", backward);

    provenance_clear();
    assert(provenance_count() == 0);
    assert(!provenance_get(first, &record));
    printf("PASS: clearing records\n");

    uint32_t oldest_id = 0;
    uint32_t newest_id = 0;

    for (uint32_t i = 0; i < PROVENANCE_MAX_RECORDS + 1; i++)
    {
        provenance_entity_ref_t source = {PROVENANCE_ENTITY_PROCESS, i + 1};
        provenance_entity_ref_t target = {PROVENANCE_ENTITY_FILE, i + 1};

        uint32_t id = provenance_record(source, target, PROVENANCE_RELATION_CREATED, PROVENANCE_REASON_CREATION);

        if (i == 0)
            oldest_id = id;

        newest_id = id;
    }

    assert(provenance_count() == PROVENANCE_MAX_RECORDS);
    assert(!provenance_get(oldest_id, &record));
    assert(provenance_get(newest_id, &record));

    printf("PASS: full buffer and oldest-record eviction\n");

    provenance_clear();
    assert(provenance_count() == 0);
    printf("PASS: final cleanup\n");

    printf("ALL PROVENANCE TESTS PASSED\n");

    return 0;
}