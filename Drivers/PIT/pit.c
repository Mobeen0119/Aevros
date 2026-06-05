#include "pit.h"
#include "../../Kernel/io.h"
#include "../../Kernel/Paging/isr.h"
#define TIME_SLICE 60

 timer_clicks = 0;

void pit_init(uint32_t frequency)
{

    uint32_t divisor = 1193182 / frequency;

    outb(0x43, 0x36);

    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)(divisor >> 8) & 0xFF);
}

int timer_callback(register_t *regs)
{
    if (++timer_clicks % TIME_SLICE == 0)
        schedule();
    return 0;
}