#ifndef AEVROS_PANIC_H
#define AEVROS_PANIC_H

#include "../../Paging/isr.h"
#include <stdint.h>

void aevros_panic(const char* reason,register_t* regs);
void decode_fault(uint32_t err, uint32_t addr, const char **what, const char **verdict);

#define KPANIC(msg)  aevros_panic(msg,NULL);

#endif