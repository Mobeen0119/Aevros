#ifndef LOCKBOX_H
#define LOCKBOX_H

#include <stdint.h>

#define LOCKBOX_CAPACITY 64
#define LOCKBOX_MAX_PER_IP 8
#define LOCKBOX_MAX_BUFFERED 4096

typedef enum
{
    LOCKBOX_OK = 0,
    LOCKBOX_REJECT_TABLE_FULL,
    LOCKBOX_REJECT_IP_QUOTA,
    LOCKBOX_REJECT_ALREADY_EXISTS,
} lockbox_result_t;


typedef struct {
    uint16_t local_port;
    uint8_t remote_ip[4];
    uint16_t remote_port;
    uint8_t protocol;
    uint32_t buffered_bytes;
    int in_use;
}lockbox_entry_t;

lockbox_result_t lockbox_claim(uint16_t local_port, const uint8_t remote_ip[4], uint16_t remote_port,
                                uint8_t protocol, uint32_t *out_id);

int lockbox_deposit(uint32_t id, uint16_t bytes);

void lockbox_release(uint32_t id);

uint32_t lockbox_active_count(void);

lockbox_result_t lockbox_listen(uint16_t local_port, uint8_t protocol, uint32_t *out_id);

uint32_t lockbox_find_listener(uint16_t local_port, uint8_t protocol);

uint32_t lockbox_rejected_count(void);

const char *lockbox_result_string(lockbox_result_t r);

uint32_t lockbox_find_connection(uint16_t local_port, const uint8_t remote_ip[4], uint16_t remote_port, uint8_t protocol);

#endif




