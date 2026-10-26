#ifndef CRITICAL_H
#define CRITICAL_H

#include <stdint.h>

static inline uint32_t critical_enter(void)
{
    uint32_t flags;
    asm volatile("pushf; pop %0; cli" : "=r"(flags));
    return flags;
}

static inline void critical_leave(uint32_t flags)
{
    asm volatile("push %0; popf" : : "r"(flags) : "memory");
}

#endif