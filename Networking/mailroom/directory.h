#ifndef DIRECTORY_H
#define DIRECTORY_H

#include <stdint.h>

typedef void (*protocol_handler_t)(const uint8_t *payload, uint16_t length);

typedef struct
{
    uint16_t ethertype;
    protocol_handler_t handler;
    const char *name;
} directory_entry_t;

#define DIRECTORY_ENTRY(type, fn, label) \
    __attribute__((section("aevros_directory"), used)) static const directory_entry_t _dirent_##fn = {(type), (fn), (label)}

#endif