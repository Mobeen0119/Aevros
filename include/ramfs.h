#ifndef RAMFS_H
#define RAMFS_H
#include <stdint.h>
#include "vfs.h"

typedef struct
{
    uint8_t *data;
    uint32_t capacity;
} ramfs_inode_t;

void ramfs_init(void);
uint32_t ramfs_read(dentry_t *dentry, uint32_t offset, uint32_t size, uint8_t *buffer);
uint32_t ramfs_write(dentry_t *dentry, uint32_t offset, uint32_t size, uint8_t *buffer);

dentry_t *ramfs_create_files(dentry_t *parent, const char *name);
int ramfs_expand(ramfs_inode_t* ram,uint32_t needed);
dentry_t *ramfs_mkdir(dentry_t *parent, const char *name);
#endif
