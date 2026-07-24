#ifndef SWITCHBOARD_H
#define SWITCHBOARD_H

#include <stdint.h>

#define SWITCHBOARD_NONE 0xFFFFFFFFu

int switchboard_bind(uint16_t port);

uint32_t switchboard_accept(uint16_t port);

#endif