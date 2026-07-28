#ifndef LOTTERY_H
#define LOTTERY_H

#include <stdint.h>


void lottery_init(void); 

uint32_t lottery_draw_isn(const uint8_t local_ip[4], uint16_t local_port,const uint8_t remote_ip[4], uint16_t remote_port);

#endif