#pragma once
#include <stdint.h>

void pic_remap();
void pic_eoi(uint8_t irq);
void pic_unmask_irq(uint8_t irq);