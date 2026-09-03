#include "fragment6.h"
#include "../../Lib/string.h"
#include "../../Lib/kprintf.h"
#include "../../kernel/Process/task.h"

#define UNITS (FRAGMENT6_MAX_TOTAL_BYTES / 8)
#define UNIT_BYTES ((UNITS + 7) / 8) // bitmap storage, 8 units per byte

typedef struct
{
    uint8_t src_ip[16], dst_ip[16];
    uint32_t id;
    uint8_t next_header;
    uint8_t buf[FRAGMENT6_MAX_TOTAL_BYTES];
    uint8_t unit_bitmap[UNIT_BYTES];
    uint16_t total_len;
    int have_last;
    uint32_t created_tick;
    int in_use;
} reassembly_t;

static reassembly_t slots[FRAGMENT6_MAX_REASSEMBLIES];
static uint32_t completed_count, overlap_count, timeout_count;

static int bit_test(const uint8_t *bitmap, int unit)
{
    return (bitmap[unit / 8] >> (unit % 8)) & 1;
}

static void bit_set(uint8_t *bitmap, int unit)
{
    bitmap[unit / 8] |= (uint8_t)(1 << (unit % 8));
}

static int range_overlaps(const uint8_t *bitmap, int start, int end)
{
    for (int i = start; i < end; i++)
        if (bit_test(bitmap, i))
            return 1;
    return 0;
}

static reassembly_t *find_slot(const uint8_t src_ip[16], const uint8_t dst_ip[16], uint32_t id)
{
    for (int i = 0; i < FRAGMENT6_MAX_REASSEMBLIES; i++)
        if (slots[i].in_use && slots[i].id == id &&
            memcmp(slots[i].src_ip, src_ip, 16) == 0 && memcmp(slots[i].dst_ip, dst_ip, 16) == 0)
            return &slots[i];
    return 0;
}

static void abort_slot(reassembly_t *s)
{
    s->in_use = 0;
}

int fragment6_receive(const uint8_t src_ip[16], const uint8_t dst_ip[16],
                       const uint8_t *fragment_header, uint16_t remaining_len,uint8_t *out_next_header, uint16_t *out_len, uint8_t *out_buf)
{
    if (remaining_len < 8)
    {
        kprintf("[Fragment6] fragment header truncated, dropping\n");
        return 0;
    }

    uint8_t frag_next_header = fragment_header[0];
    uint16_t offset_flags = (uint16_t)((fragment_header[2] << 8) | fragment_header[3]);
    uint16_t offset_units = offset_flags >> 3;
    int more_fragments = offset_flags & 0x1;
    uint32_t id = ((uint32_t)fragment_header[4] << 24) | ((uint32_t)fragment_header[5] << 16) |
                  ((uint32_t)fragment_header[6] << 8) | fragment_header[7];

    const uint8_t *data = fragment_header + 8;
    uint16_t data_len = (uint16_t)(remaining_len - 8);
    uint16_t offset_bytes = (uint16_t)(offset_units * 8);

    if (more_fragments && (data_len % 8) != 0)
    {
        // RFC 8200 4.5: every fragment but the last must carry a multiple of 8 bytes
        kprintf("[Fragment6] non-final fragment has a non-8-byte-aligned length, dropping\n");
        return 0;
    }

    if ((uint32_t)offset_bytes + data_len > FRAGMENT6_MAX_TOTAL_BYTES)
    {
        kprintf("[Fragment6] reassembled packet would exceed our %d-byte cap, dropping\n", FRAGMENT6_MAX_TOTAL_BYTES);
        reassembly_t *existing = find_slot(src_ip, dst_ip, id);
        if (existing)
            abort_slot(existing); // can't be completed correctly for us now regardless
        return 0;
    }

    reassembly_t *s = find_slot(src_ip, dst_ip, id);

    if (!s)
    {
        for (int i = 0; i < FRAGMENT6_MAX_REASSEMBLIES; i++)
            if (!slots[i].in_use)
            {
                s = &slots[i];
                break;
            }
        if (!s)
        {
            kprintf("[Fragment6] reassembly table full, dropping fragment\n");
            return 0;
        }

        memcpy(s->src_ip, src_ip, 16);
        memcpy(s->dst_ip, dst_ip, 16);
        s->id = id;
        s->next_header = frag_next_header;
        memset(s->unit_bitmap, 0, sizeof(s->unit_bitmap));
        s->total_len = 0;
        s->have_last = 0;
        s->created_tick = get_ticks();
        s->in_use = 1;
    }
    else if (s->next_header != frag_next_header)
    {
        // fragments of one packet must all agree on what follows them - a
        // mismatch here is either corruption or someone trying to confuse reassembly
        kprintf("[Fragment6] fragment's next_header disagrees with earlier fragments, aborting reassembly\n");
        abort_slot(s);
        return 0;
    }

    int start_unit = offset_units;
    // round the tail up to a whole unit for the overlap check - conservative on
    // purpose (see header note): only ever makes us reject something borderline,
    // never lets a real overlap slip through
    int end_unit = (int)((offset_bytes + data_len + 7) / 8);
    if (end_unit > UNITS)
        end_unit = UNITS;

    if (range_overlaps(s->unit_bitmap, start_unit, end_unit))
    {
        kprintf("[Fragment6] overlapping fragment detected, discarding entire reassembly (RFC 5722)\n");
        abort_slot(s);
        overlap_count++;
        return 0;
    }

    memcpy(s->buf + offset_bytes, data, data_len);
    for (int i = start_unit; i < end_unit; i++)
        bit_set(s->unit_bitmap, i);

    if (!more_fragments)
    {
        if (s->have_last)
        {
            // two different fragments both claiming to be the last one - ambiguous, don't trust either
            kprintf("[Fragment6] conflicting final fragment, aborting reassembly\n");
            abort_slot(s);
            return 0;
        }
        s->have_last = 1;
        s->total_len = (uint16_t)(offset_bytes + data_len);
    }

    if (!s->have_last)
        return 0;

    int needed_units = (s->total_len + 7) / 8;
    for (int i = 0; i < needed_units; i++)
        if (!bit_test(s->unit_bitmap, i))
            return 0; // still missing pieces

    memcpy(out_buf, s->buf, s->total_len);
    *out_len = s->total_len;
    *out_next_header = s->next_header;

    abort_slot(s);
    completed_count++;
    return 1;
}

void fragment6_tick(void)
{
    uint32_t now = get_ticks();

    for (int i = 0; i < FRAGMENT6_MAX_REASSEMBLIES; i++)
    {
        if (slots[i].in_use && now - slots[i].created_tick > FRAGMENT6_TIMEOUT_TICKS)
        {
            kprintf("[Fragment6] reassembly timed out, giving up on it\n");
            slots[i].in_use = 0;
            timeout_count++;
        }
    }
}

uint32_t fragment6_completed_count(void)
{
    return completed_count;
}

uint32_t fragment6_overlap_count(void)
{
    return overlap_count;
}

uint32_t fragment6_timeout_count(void)
{
    return timeout_count;
}