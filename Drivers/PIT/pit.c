#include "pit.h"
#include "../../Kernel/io.h"
#include "../../Kernel/Paging/isr.h"
#include "../../kernel/Process/task.h"
#include "../../Lib/kprintf.h"
#include "../../Include/screen.h"
#include "../../kernel/Process/Quarantine/Quarantine.h"
#define TIME_SLICE 60
#define QUARANTINE_SCAN_INTERVAL 600

volatile uint32_t timer_clicks = 0;

void pit_init(uint32_t frequency)
{

    uint32_t divisor = 1193182 / frequency;

    outb(0x43, 0x36);

    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)(divisor >> 8) & 0xFF);
}

int timer_callback(register_t *regs)
{
    (void)regs;
    kprintf("yh tw chal rha"); 
    
    if (++timer_clicks % TIME_SLICE == 0)
        schedule();

    if (timer_clicks % QUARANTINE_SCAN_INTERVAL == 0)
        quarantine_check_and_act();

  return 0;
}
