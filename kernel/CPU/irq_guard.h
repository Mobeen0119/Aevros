#ifndef IRQ_GUARD_H
#define IRQ_GUARD_H

#include <stdint.h>

#ifdef AEVROS_HOST_TEST
static inline uint32_t irq_guard_save(void)
{
    return 0;
}
static inline void irq_guard_restore(uint32_t flags)
{

    (void)flags;
}
#else
static inline uint32_t irq_guard_save(void)
{
    uint32_t flags;
    __asm__ volatile("pushf\n\tpop %0\n\tcli" : "=r"(flags) : : "memory");
    return flags;
}

static inline void irq_guard_restore(uint32_t flags)
{
    if (flags & 0x200)
        __asm__ volatile("sti" : : : "memory");
}
#endif

#endif