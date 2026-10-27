#ifndef E1000_H
#define E1000_H

#include <stdint.h>
#include "frontdesk.h"

int e1000_probe(frontdesk_state_t *st);

int e1000_send(const void *data, uint16_t length);

void e1000_irq(void);

#endif