#ifndef IP6_DIRECTORY_H
#define IP6_DIRECTORY_H
#include <stdint.h>

typedef void (*ip6_protocol_handler_t)(const uint8_t *payload, uint16_t length, const uint8_t src_ip[16], const uint8_t dst_ip[16]);

typedef struct
{
    uint8_t next_header;
    ip6_protocol_handler_t handler;
    const char *name;

} ip6_directory_entry_t;

#define IP6_DIRECTORY_ENTRY(nh, fn, label) __attribute__((section(".aevros_ip6_directory"), used)) static const ip6_directory_entry_t _ip6dirent_##fn = {(nh), (fn), (label)}

#endif