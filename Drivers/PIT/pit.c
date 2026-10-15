#include "pit.h"
#include "../../kernel/io.h"
#include "../../kernel/Paging/isr.h"
#include "../../kernel/Process/task.h"
#include "../../Lib/kprintf.h"
#include "../../Include/screen.h"
#include "../../kernel/Process/Quarantine/Quarantine.h"
#include "../../Networking/FrontDesk/frontdesk.h"
#include "../../Networking/Concierge/concierge.h"
#include "../../Networking/NetWatch/net_observe.h"
#define DEFAULT_TIME_SLICE 60 // ticks per slice (0.6 s at 100 Hz)

static volatile uint32_t time_slice = DEFAULT_TIME_SLICE;

void pit_set_time_slice(uint32_t ticks)
{
    time_slice = ticks ? ticks : DEFAULT_TIME_SLICE;
}
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
    ++timer_clicks;

    if (timer_clicks % time_slice == 0)
        schedule();

    if (timer_clicks % QUARANTINE_SCAN_INTERVAL == 0)
        quarantine_check_and_act();

    const frontdesk_state_t *nic = frontdesk_get_state();
    if (nic->present)
        concierge_maybe_tick(nic->mac); // networking's periodic maintenance heartbeat

    netobserve_drain();

    return 0;
}