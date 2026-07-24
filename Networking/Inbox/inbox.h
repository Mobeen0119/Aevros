#ifndef INBOX_H
#define INBOX_H

#include <stdint.h>


int inbox_deposit(uint32_t conn_id, const uint8_t *payload, uint16_t len);

uint16_t inbox_read(uint32_t conn_id, uint8_t *out, uint16_t max_len);

uint16_t inbox_available(uint32_t conn_id);

#endif