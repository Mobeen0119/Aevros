#include "dev.h"
#include "../../Lib/string.h"
#include "../../Include/vfs.h"
#include "../Memory/pmm.h"

static devfs_device_t devfs_table[MAX_DEVICES];
static int devfs_count = 0;
static dentry_t *devfs_root = NULL;

void devfs_init()
{
    devfs_root = (dentry_t *)kmalloc_raw(sizeof(dentry_t));
    memset(devfs_root, 0, sizeof(dentry_t));
    devfs_root->name = "dev";
    devfs_root->parent = vfs_root;

    inode_t *ino = (inode_t *)kmalloc_raw(sizeof(inode_t));
    memset(ino, 0, sizeof(inode_t));
    ino->flags = VFS_DIR;
    devfs_root->inode = ino;

    uint32_t bucket = dentry_hash("dev");
    devfs_root->hash_next = vfs_root->hash_bucket[bucket];
    vfs_root->hash_bucket[bucket] = devfs_root;
}

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
    if (devfs_root)
    {
        dentry_t *d = (dentry_t *)kmalloc_raw(sizeof(dentry_t));
        memset(d, 0, sizeof(dentry_t));
        d->name = (char *)name;
        d->inode = inode;
        d->parent = devfs_root;
        uint32_t bucket = dentry_hash(name);
        d->hash_next = devfs_root->hash_bucket[bucket];
        devfs_root->hash_bucket[bucket] = d;
    }
    return VFS_OK;
}

inode_t *devfs_get(const char *name)
{
    if (!name)
        return NULL;

    for (int i = 0; i < devfs_count; i++)
    {
        if (strcmp(devfs_table[i].name, name) == 0)
            return devfs_table[i].inode;
    }
    return NULL;
}
