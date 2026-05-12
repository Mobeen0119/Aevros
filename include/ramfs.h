#define RAMFS_H
#include <stdint.h>

typedef struct
{
    uint8_t *data;
    uint32_t capacity;
} ramfs_inode_t;

int ramfs_read(inode_t *inode, uint32_t offset, uint32_t size, uint8_t *buffer);
int ramfs_write(inode_t *inode, uint32_t offset, uint32_t size, uint8_t *buffer);

dentry_t *ramfs_create_files(dentry_t *parent, const char *name);
int ramfs_expand(ramfs_inode_t* ram,uint32_t needed);
