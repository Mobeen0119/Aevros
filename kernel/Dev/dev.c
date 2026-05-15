#include "dev.h"
#include "../../Lib/string.c"


int devfs_register(const char *name, inode_t *inode)
{
    if (!name || !inode)
        return VFS_ERR;

    if (devfs_count >= MAX_DEVICES)
        return VFS_ERR;

    for (int i = 0; i < devfs_count; i++)
    {
        if (match_seg(devfs_table[i].name, name, strlen(name)))
            return VFS_ERR;
    }
    devfs_table[devfs_count].name = strdup(name);
    devfs_table[devfs_count].inode = inode;

    devfs_count++;

    return VFS_OK;
}

inode_t *devfs_get(const char *name)
{
    if (!name)
        return VFS_ERR;

    for (int i = 0; i < devfs_count; i++)
    {
        if (match_seg(devfs_table[i].name, name, strlen(name)))
            return devfs_table[i].inode;
    }
    return VFS_OK;
}
