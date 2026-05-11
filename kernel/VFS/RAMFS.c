#include <stdint.h>
#include "..\..\Include\ramfs.h"
#include "..\..\Include\vfs.h"
#include "..\Memory\pmm.h"

int ramfs_read(inode_t *inode, uint32_t offset, uint32_t size, uint8_t *buffer)
{
    ramfs_inode_t *ram = inode->fs_private;

    if (!ram)
        return -1;

    if (offset > inode->size)
        return 0;

    if (offset + size > inode->size)
        size = inode->size - offset;

    memcpy(buffer, ram->data + offset, size);
    return size;
}

int ramfs_write(inode_t *inode, uint32_t offset, uint32_t size, uint8_t *buffer)
{
    ramfs_inode_t *ram = inode->fs_private;

    if (!ram)
        return -1;

    memcpy(ram->data + offset, buffer, size);

    if (offset + size > inode->size)
        inode->size = offset + size;

    return size;
}