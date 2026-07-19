#ifndef FRONTDESK_H
#define FRONTDESK_H

#include <stdint.h>

typedef struct {
    int present;
    uint8_t mac[6];
    uint16_t io_base;
    uint8_t irq_line;
    uint32_t packets_received;
    uint32_t packets_send;
    uint32_t rx_errors;

}frontdesk_state_t;

void frontdesk_init(void);
const frontdesk_state_t *frontdesk_get_state(void);
int frontdesk_send(const void *data, uint16_t length);
void frontdesk_irq_handler(void);


#endif