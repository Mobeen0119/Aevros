#include "../../Include/vfs.h"


typedef struct
{
    char *name;
    inode_t *inode;
} devfs_device_t;



void devfs_init();

int devfs_register(const char *name, inode_t *inode);
inode_t *devfs_get(const char *name);