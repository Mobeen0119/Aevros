#ifndef LOCKBOX6_H
#define LOCKBOX6_H

#include <stdint.h>

#define LOCKBOX6_CAPACITY 32
#define LOCKBOX6_MAX_PER_IP 8
#define LOCKBOX6_MAX_BUFFERED 4096

typedef enum
{
    LOCKBOX6_OK = 0,
    LOCKBOX6_REJECT_TABLE_FULL,
    LOCKBOX6_REJECT_IP_QUOTA,
    LOCKBOX6_REJECT_ALREADY_EXISTS,
} lockbox6_result_t;

typedef struct
{
    uint16_t local_port;
    uint8_t remote_ip[16];
    uint16_t remote_port;
    uint8_t protocol;
    uint32_t buffered_bytes;
    int in_use;
} lockbox6_entry_t;

lockbox6_result_t lockbox6_claim(uint16_t local_port, const uint8_t remote_ip[16], uint16_t remote_port,
                                 uint8_t protocol, uint32_t *out_id);

int lockbox6_deposit(uint32_t id, uint16_t bytes);

void lockbox6_release(uint32_t id);

uint32_t lockbox6_active_count(void);

lockbox6_result_t lockbox6_listen(uint16_t local_port, uint8_t protocol, uint32_t *out_id);

uint32_t lockbox6_find_listener(uint16_t local_port, uint8_t protocol);

uint32_t lockbox6_rejected_count(void);

uint32_t lockbox6_next_for_port(uint16_t local_port, uint8_t protocol, uint32_t after_id);

const char *lockbox6_result_string(lockbox6_result_t r);

void lockbox6_consume(uint32_t id, uint16_t bytes);

int lockbox6_get_tuple(uint32_t id, uint16_t *local_port, uint8_t remote_ip[16], uint16_t *remote_port);

uint32_t lockbox6_get_generation(uint32_t id);

uint32_t lockbox6_find_connection(uint16_t local_port, const uint8_t remote_ip[16], uint16_t remote_port, uint8_t protocol);

#endif
