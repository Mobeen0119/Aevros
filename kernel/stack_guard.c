#include <stdint.h>
#include <stddef.h>
#include "Paging/Aevros_Panic/aevros_panic.h"


uintptr_t __stack_chk_guard = 0xDEADC0DE;

__attribute__((noreturn)) void __stack_chk_fail(void)
{
    aevros_panic("Stack smashing detected  canary overwritten", NULL);
    __builtin_unreachable();
}
