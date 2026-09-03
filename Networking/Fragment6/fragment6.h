#ifndef FRAGMENT6_H
#define FRAGMENT6_H
#include <stdint.h>

#define FRAGMENT6_MAX_REASSEMBLIES 4
#define FRAGMENT6_MAX_TOTAL_BYTES 2048 // matches the small fixed-buffer scale used everywhere else in this stack
#define FRAGMENT6_TIMEOUT_TICKS 500    // RFC 8200: timer starts at the first fragment, never refreshed

int fragment6_receive(const uint8_t src_ip[16], const uint8_t dst_ip[16],const uint8_t *fragment_header, uint16_t remaining_len,
                       uint8_t *out_next_header, uint16_t *out_len, uint8_t *out_buf);

void fragment6_tick(void);

uint32_t fragment6_completed_count(void);
uint32_t fragment6_overlap_count(void);
uint32_t fragment6_timeout_count(void);

#endif