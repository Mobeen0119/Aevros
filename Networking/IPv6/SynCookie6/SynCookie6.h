#ifndef SYNCOOKIE6_H
#define SYNCOOKIE6_H
#include <stdint.h>

#define SYNCOOKIE6_LOAD_NUM 3
#define SYNCOOKIE6_LOAD_DEN 4

int syncookie6_should_activate(void);

uint32_t syncookie6_generate(const uint8_t src_ip[16], uint16_t src_port, const uint8_t dst_ip[16], uint16_t dst_port);

int syncookie6_validate(const uint8_t src_ip[16], uint16_t src_port, const uint8_t dst_ip[16], uint16_t dst_port, uint32_t ack_num);

#endif