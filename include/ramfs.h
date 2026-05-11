#define RAMFS_H
#include <stdint.h>

typedef struct
{
    uint8_t *data;
    uint32_t capacity;
} ramfs_inode_t;