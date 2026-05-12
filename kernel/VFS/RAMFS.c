#include <stdint.h>
#include "..\..\Include\ramfs.h"
#include "..\..\Include\vfs.h"
#include "..\Memory\pmm.h"
#include "..\Memory\kheap.c"

vfs_ops_t ramfs_ops = {
    .read = ramfs_read,
    .write = ramfs_write};

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

const char *basename(const char *path)
{
    const char *last = path;

    while (*path)
    {
        if (path == '/')
            last = path + 1;
        path++;
    }
    return last;
}

void parent_dirname(const char *path, char *parent)
{
    int last = -1;
    for (int i = 0; path[i]; i++)
    {
        if (path == '/')
            last = i;
    }
    if (last <= 0)
    {
        parent[0] = '/';
        parent[1] = '\0';
        return;
    }

    for (int i = 0; i < last; i++)
    {
        parent[i] = path[i];
    }
    parent[last] = '\0';
}