#ifndef PAGE_FAULT
#define PAGE_FAULT
#include "isr.h"

void page_fault_handler(struct registers* reg);

#endif