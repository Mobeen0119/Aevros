#include <stdint.h>
#include "../../Include/ramfs.h"
#include "../../Include/vfs.h"
#include "../Memory/pmm.h"
#include "../Memory/kheap.h"
#include "../../Lib/string.h"

vfs_ops_t ramfs_ops = {
    .read = ramfs_read,
    .write = ramfs_write};

void ramfs_init()
{
    vfs_root->inode->ops = &ramfs_ops;
}

uint32_t ramfs_read(dentry_t *dentry, uint32_t offset, uint32_t size, uint8_t *buffer)
{
    ramfs_inode_t *ram = dentry->inode->fs_private;

    if (!ram)
        return -1;

    if (offset > dentry->inode->size)
        return 0;

    if (offset + size > dentry->inode->size)
        size = dentry->inode->size - offset;

    memcpy(buffer, ram->data + offset, size);
    return size;
}

uint32_t ramfs_write(dentry_t *dentry, uint32_t offset, uint32_t size, uint8_t *buffer)
{
    ramfs_inode_t *ram = dentry->inode->fs_private;
    if (!ram)
        return -1;

    if (offset + size > ram->capacity)
        if (ramfs_expand(ram, offset + size) < 0)
            return -1;

    memcpy(ram->data + offset, buffer, size);

    if (offset + size > dentry->inode->size)
        dentry->inode->size = offset + size;

    return size;
}

dentry_t *ramfs_create_files(dentry_t *parent, const char *name)
{

    dentry_t *child = kmalloc_raw(sizeof(dentry_t));
    inode_t *inode = kmalloc_raw(sizeof(inode_t));
    ramfs_inode_t *ram = kmalloc_raw(sizeof(ramfs_inode_t));

    if (!ram || !child || !inode)
        return 0;

    memset(child, 0, sizeof(dentry_t));
    memset(inode, 0, sizeof(inode_t));
    memset(ram, 0, sizeof(ramfs_inode_t));

    ram->capacity = 4096;
    ram->data = kmalloc_raw(ram->capacity);
    if (!ram->data){
         kfree_raw(ram);
    kfree_raw(inode);
    kfree_raw(child);
    return NULL;
    }
        
    inode->size = 0;
    inode->flags = VFS_FILE;
    inode->ops = &ramfs_ops;
    inode->fs_private = ram;
    inode->ref_count = 0;

    child->name = strdup(name);
    child->inode = inode;
    child->parent = parent;

    uint32_t bucket = dentry_hash(child->name);
    child->hash_next = parent->hash_bucket[bucket];
    parent->hash_bucket[bucket] = child;

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

    uint32_t *new_data = kmalloc_raw(new_cap);

    if (!new_data)
        return VFS_ERR;

    memcpy(new_data, ram->data, ram->capacity);
    kfree_raw(ram->data);

    ram->data = new_data;
    ram->capacity = new_cap;

    return VFS_OK;
}

dentry_t *ramfs_mkdir(dentry_t *parent, const char *name)
{
    if (!parent || !name)
        return 0;

    dentry_t *dentry = kmalloc_raw(sizeof(dentry_t));
    inode_t *inode = kmalloc_raw(sizeof(inode_t));

    if (!dentry || !inode)
        return 0;

    memset(dentry, 0, sizeof(dentry_t));
    memset(inode, 0, sizeof(inode_t));

    inode->flags = VFS_DIR;
    inode->size = 0;
    inode->ops = NULL;
    inode->fs_private = NULL;

    dentry->name = strdup(name);
    dentry->inode = inode;
    dentry->parent = parent;

    uint32_t bucket = dentry_hash(name);
    dentry->hash_next = parent->hash_bucket[bucket];
    parent->hash_bucket[bucket] = dentry;

    return dentry;
}
