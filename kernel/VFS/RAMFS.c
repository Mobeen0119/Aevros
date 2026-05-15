#include <stdint.h>
#include "..\..\Include\ramfs.h"
#include "..\..\Include\vfs.h"
#include "..\Memory\pmm.h"
#include "..\Memory\kheap.c"
#include "..\..\Lib\string.c"

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
        if (ramfs_expand(ram, offset + size) < 0)
            return -1;

    return size;
}

dentry_t *ramfs_create_files(dentry_t *parent, const char *name)
{

    dentry_t *child = kmalloc(sizeof(dentry_t));
    inode_t *inode = kmalloc(sizeof(inode_t));
    ramfs_inode_t *ram = kmalloc(sizeof(ramfs_inode_t));

    if (!ram || !child || !inode)
        return 0;

    memset(child, 0, sizeof(dentry_t));
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

    child->name = strdup(name);
    child->inode = inode;
    child->parent = parent;

   uint32_t bucket=dentry_hash(child->name);
    child->next=parent->hash_bucket[bucket];
    parent->hash_bucket[bucket]=child;
    
    return child;
}

int ramfs_expand(ramfs_inode_t *ram, uint32_t needed)
{

    if (!ram)
        return -1;
    if (needed <= ram->capacity)
    {
        return 0;
    }
    uint32_t new_cap = ram->capacity;

    while (new_cap < needed)
        new_cap *= 2;

    uint32_t *new_data = kmalloc(new_cap);

    if (!new_data)
        return -1;

    memcpy(new_data, ram->data, ram->capacity);
    kfree(ram->data);

    ram->data = new_data;
    ram->capacity = new_cap;

    return 0;
}

dentry_t *ramfs_mkdir(dentry_t *parent, const char *name)
{
    if (!parent || !name)
        return 0;

    dentry_t *dentry = kmalloc(sizeof(dentry_t));
    inode_t *inode = kmalloc(sizeof(inode_t));

    if (!dentry || !inode)
        return 0;

    memset(dentry, 0, sizeof(dentry_t));
    memset(inode, 0, sizeof(inode_t));

    inode->flags = VFS_DIR;
    inode->fs_private = inode->ops = inode->size = 0;

    dentry->name = strdup(name);
    dentry->inode = inode;
    dentry->parent = parent;

    dentry->children = 0;
    dentry->next = parent->children;

    parent->children = dentry;

    return dentry;
}