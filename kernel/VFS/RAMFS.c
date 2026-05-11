#include <stdint.h>
#include "..\..\Include\ramfs.h"
#include "..\..\Include\vfs.h"
#include "..\Memory\pmm.h"
#include "..\Memory\kheap.c"

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

dentry_t *ramfs_create_files(dentry_t *parent, const char *name)
{

    dentry_t *dentry = kmalloc(sizeof(dentry_t));
    inode_t *inode = kmalloc(sizeof(inode_t));
    ramfs_inode_t *ram = kmalloc(sizeof(ramfs_inode_t));

    if (!ram || !dentry || !inode)
        return 0;

    memset(dentry, 0, sizeof(dentry_t));
    memset(inode, 0, sizeof(inode_t));
    memset(ram, 0, sizeof(ramfs_inode_t));

    ram->capacity = 4096;
    ram->data = kmalloc(ram->capacity);
    if (!ram->data)
        return 0;

    inode->size = 0;
    inode->flags = VFS_FILE;
    inode->ops = &ramfs_ops;
    inode->fs_private = ram;
    inode->ref_count = 0;

    dentry->name = strdup(name);
    dentry->inode = inode;
    dentry->parent = parent;

    return dentry;
}