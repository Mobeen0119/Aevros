#ifndef SYNCOOKIE_H
#define SYNCOOKIE_H
#include <stdint.h>

#define SYNCOOKIE_LOAD_NUM 3
#define SYNCOOKIE_LOAD_DEN 4

int syncookie_should_activate(void);

uint32_t syncookie_generate(const uint8_t src_ip[4], uint16_t src_port,const uint8_t dst_ip[4], uint16_t dst_port);

int syncookie_validate(const uint8_t src_ip[4], uint16_t src_port,const uint8_t dst_ip[4], uint16_t dst_port, uint32_t ack_num);
#endif