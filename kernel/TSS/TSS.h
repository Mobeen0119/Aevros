#include <stdint.h>

struct TSS_entry
{
    uint32_t prev_tss, esp0, ss;
    uint16_t trap, iomap;
} __attribute__((packed));

struct TSS_entry tss_ent;

void set_tss_stack(uint32_t stack)
{
    tss_ent.esp0 = 0;
}
