#include "provenance.h"
#include "../Include/terminal.h"
#include "../Lib/kprintf.h"

static const char *provenance_entity_name(provenance_entity_t type)
{
    switch (type)
    {
    case PROVENANCE_ENTITY_INTERRUPT:
        return "INTERRUPT";
    case PROVENANCE_ENTITY_DRIVER:
        return "DRIVER";
    case PROVENANCE_ENTITY_PROCESS:
        return "PROCESS";
    case PROVENANCE_ENTITY_MEMORY:
        return "MEMORY";
    case PROVENANCE_ENTITY_NETWORK:
        return "NETWORK";
    case PROVENANCE_ENTITY_FILE:
        return "FILE";
    case PROVENANCE_ENTITY_DEVICE:
        return "DEVICE";
    case PROVENANCE_ENTITY_OPERATION:
        return "OPERATION";

    default:
        return "UNKNOWN";
    }
}

static const char *provenance_relation_name(provenance_relation_t relation)
{
    switch (relation)
    {
    case PROVENANCE_RELATION_CAUSED:
        return "CAUSED";
    case PROVENANCE_RELATION_AFFECTED:
        return "AFFECTED";
    case PROVENANCE_RELATION_CREATED:
        return "CREATED";
    case PROVENANCE_RELATION_DESTROYED:
        return "DESTROYED";
    case PROVENANCE_RELATION_OWNED:
        return "OWNED";
    case PROVENANCE_RELATION_TRIGGERED:
        return "TRIGGERED";
    case PROVENANCE_RELATION_DEPENDED_ON:
        return "DEPENDED_ON";
    case PROVENANCE_RELATION_RESPONDED_TO:
        return "RESPONDED_TO";

    default:
        return "UNKNOWN";
    }
}

static const char *provenance_reason_name(provenance_reason_t reason)
{
    switch (reason)
    {
    case PROVENANCE_REASON_INTERRUPT:
        return "INTERRUPT";
    case PROVENANCE_REASON_SCHEDULE:
        return "SCHEDULE";
    case PROVENANCE_REASON_ALLOCATION:
        return "ALLOCATION";
    case PROVENANCE_REASON_DEALLOCATION:
        return "DEALLOCATION";
    case PROVENANCE_REASON_CREATION:
        return "CREATION";
    case PROVENANCE_REASON_DESTRUCTION:
        return "DESTRUCTION";
    case PROVENANCE_REASON_REQUEST:
        return "REQUEST";
    case PROVENANCE_REASON_RESPONSE:
        return "RESPONSE";
    case PROVENANCE_REASON_DELIVERY:
        return "DELIVERY";
    case PROVENANCE_REASON_FAILURE:
        return "FAILURE";
    case PROVENANCE_REASON_DEPENDENCY:
        return "DEPENDENCY";
    case PROVENANCE_REASON_USER_ACTION:
        return "USER_ACTION";
    case PROVENANCE_REASON_SYSTEM_ACTION:
        return "SYSTEM_ACTION";

    default:
        return "NONE";
    }
}

void provenance_report(void)
{
    static provenance_snapshot_t snapshot;

    provenance_get_snapshot(&snapshot);

    set_color(VGA_CYAN, VGA_BLACK);

    kprintf("\n PROVENANCE REPORT\n");
    kprintf(" -----------------\n");
    reset_color();

    if (snapshot.record_count == 0)
    {
        kprintf("No provenance relationships recorded.\n");
        return;
    }

    uint32_t shown = 0;

    for (uint32_t i = 0; i < snapshot.record_count; i++)
    {
        provenance_record_t *r = &snapshot.records[i];

        if (!r->valid)
            continue;

        kprintf("[%u] t=%u %s:%u --%s--> %s:%u\n", r->id, r->tick, provenance_entity_name(r->source.type),
                r->source.id, provenance_relation_name(r->relation), provenance_entity_name(r->target.type), r->target.id);

        kprintf("    Reason: %s\n", provenance_reason_name(r->reason));

        shown++;
    }

    kprintf("Records displayed: %u (overwritten: %u)\n", shown, snapshot.dropped);
}