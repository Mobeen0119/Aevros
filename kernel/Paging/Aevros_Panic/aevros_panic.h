#ifndef AEVROS_PANIC_H
#define AEVROS_PANIC_H

#include "../../Paging/isr.h"
#include <stdint.h>

void aevros_panic(const char* reason,register_t* regs);

#define KPANIC(msg)  aevros_panic(msg,NULL);

#endif