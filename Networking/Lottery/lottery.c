#include "lottery.h"
#include "../../kernel/Process/task.h"

static uint32_t boot_secret;
static uint32_t call_counter;

static uint32_t mix32(uint32_t x)
{
    x ^= x >> 16;
    x *= 0x85ebca6bu;
    x ^= x >> 13;
    x *= 0xc2b2ae35u;
    x ^= x >> 16;
    return x;
}

void lottery_init(void)
{
    boot_secret = mix32(get_ticks() ^ 0x9E3779B9u);
    call_counter = 0;
}

uint32_t lottery_draw_isn(const uint8_t local_ip[4], uint16_t local_port,
                          const uint8_t remote_ip[4], uint16_t remote_port)
{
    uint32_t h = boot_secret;

    h ^= mix32((uint32_t)((local_ip[0] << 24) | (local_ip[1] << 16) | (local_ip[2] << 8) | local_ip[3]));
    h = mix32(h);

    h ^= mix32((uint32_t)((remote_ip[0] << 24) | (remote_ip[1] << 16) | (remote_ip[2] << 8) | remote_ip[3]));
    h = mix32(h);

    h ^= mix32(((uint32_t)local_port << 16) | remote_port);
    h = mix32(h);

    h ^= mix32(get_ticks());
    h = mix32(h);

    h ^= mix32(call_counter++);
    h = mix32(h);

    return h;
}