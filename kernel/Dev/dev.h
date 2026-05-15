#include "../../Include/vfs.h"


typedef struct
{
    char *name;
    inode_t *inode;
} devfs_device_t;


static devfs_device_t devfs_table[MAX_DEVICES];
static int devfs_count = 0;

int devfs_register(const char *name, inode_t *inode);
inode_t *devfs_get(const char *name);