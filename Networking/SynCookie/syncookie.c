#include "syncookie.h"
#include "../LockBox/lockbox.h"
#include "../Lottery/lottery.h"
#include "../../kernel/Process/task.h"

#define TIME_BUCKET_SHIFT 5

static uint32_t secret;
static int secret_ready;

static void ensure_secret(void)
{

    if (secret_ready)
        return;

    static const uint8_t a[4] = {10, 0, 2, 15};

    static const uint8_t b[4] = {1, 2, 3, 4};

    secret = lottery_draw_isn(a, 12345, b, 54321) ^ get_ticks();

    secret_ready = 1;
}

static uint32_t mix(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{

    uint32_t h = secret;
    h = (h ^ a) * 0x9E3779B1u;
    h = (h ^ b) * 0x85EBCA77u;

    h = (h ^ c) * 0xC2B2AE3Du;
    h = (h ^ d) * 0x27D4EB2Fu;

    h ^= h >> 15;

    return h;
}

static uint32_t compute(const uint8_t src_ip[4], uint16_t src_port, const uint8_t dst_ip[4], uint16_t dst_port, uint32_t time_bucket)
{
    ensure_secret();
    uint32_t a = (uint32_t)((src_ip[0] << 24) | (src_ip[1] << 16) | (src_ip[2] << 8) | src_ip[3]);

    uint32_t b = (uint32_t)((dst_ip[0] << 24) | (dst_ip[1] << 16) | (dst_ip[2] << 8) | dst_ip[3]);
    uint32_t c = (uint32_t)(((uint32_t)src_port << 16) | dst_port);

    return mix(a, b, c, time_bucket);
}

int syncookie_should_activate(void)
{

    return lockbox_active_count() * SYNCOOKIE_LOAD_DEN >= LOCKBOX_CAPACITY * SYNCOOKIE_LOAD_NUM;
}

uint32_t syncookie_generate(const uint8_t src_ip[4], uint16_t src_port, const uint8_t dst_ip[4], uint16_t dst_port)
{
    uint32_t bucket = get_ticks() >> TIME_BUCKET_SHIFT;
    return compute(src_ip, src_port, dst_ip, dst_port, bucket);
}

int syncookie_validate(const uint8_t src_ip[4], uint16_t src_port,const uint8_t dst_ip[4], uint16_t dst_port, uint32_t ack_num)
{
    uint32_t bucket = get_ticks() >> TIME_BUCKET_SHIFT;
    uint32_t expected_seq = ack_num - 1;

    if (expected_seq == compute(src_ip, src_port, dst_ip, dst_port, bucket))
        return 1;

    if (bucket > 0 && expected_seq == compute(src_ip, src_port, dst_ip, dst_port, bucket - 1))
        return 1;
    return 0;
}
