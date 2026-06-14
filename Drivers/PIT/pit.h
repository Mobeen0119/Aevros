#pragma once
#include <stdint.h>
#include "../../kernel/Paging/isr.h"


extern volatile uint32_t timer_clicks;

void pit_init(uint32_t frequency);
 
int timer_callback(register_t* regs);