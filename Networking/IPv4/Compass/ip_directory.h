#ifndef IP_DIRECTORY_H
#define IP_DIRECTORY_H

#include <stdint.h>

typedef void (*ip_protocol_handler_t)(const uint8_t *payload, uint16_t length, const uint8_t src_ip[4], const uint8_t dst_ip[4]);

typedef struct
{
    uint8_t protocol;
    ip_protocol_handler_t handler;
    const char *name;
} ip_directory_entry_t;

#define IP_DIRECTORY_ENTRY(proto, fn, label) __attribute__((section(".aevros_ip_directory"), used)) static const ip_directory_entry_t _ipdirent_##fn = {(proto), (fn), (label)}

#endif
