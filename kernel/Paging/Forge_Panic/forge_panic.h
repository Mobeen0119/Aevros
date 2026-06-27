#ifndef FORGE_PANIC_H
#define FORGE_PANIC_H

#include "../../Paging/isr.h"
#include <stdint.h>

void forge_panic(const char* reason,register_t* regs);

#define KPANIC(msg)  forge_panic(msg,NULL);

#endif