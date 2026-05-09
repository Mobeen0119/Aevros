#include <stdint.h>
#include "../Memory/kheap.c"
#include "../Process/task.h"
#include "C:\Users\PROLINE LAPTOP STORE\ForgeOS\include\vfs.h"

dentry_t *vfs_root = 0;

uint32_t vfs_read(dentry_t *node, uint32_t offset, uint32_t size, uint8_t *buffer)
{
    if (node && node->inode && node->inode->ops && node->inode->ops->read)
    {
        return node->inode->ops->read(node->inode, offset, size, buffer);
    }
    return 0;
}

uint32_t vfs_write(dentry_t *node, uint32_t offset, uint32_t size, uint8_t *buffer)
{
    if (node && node->inode && node->inode->ops && node->inode->ops->write)
    {
        return node->inode->ops->write(node->inode, offset, size, buffer);
    }
    return 0;
}

static const char *skip_slash(const char *p)
{
    while (*p == '/')
        p++;
    return p;
}

//-----------------------a=>what exist ---b=>what asked for

static int match_seg(const char *name, const char *start, uint32_t len)
{
    uint32_t i = 0;
    while (name[i] && i < len)
    {
        if (name[i] != start[i])
            return 0;
        i++;
    }
    return (name[i] == '\0' && i == len);
}

dentry_t *vfs_lookup(dentry_t *root, const char *path)
{

    if (!root || !path)
        return 0;

    const char *p = path;
    dentry_t *current = root;

    while (*p)
    {
        while (*p == '/')
            p++;

        if (!*p)
            break;

        const char *start = p;

        while (*p && *p != '/')
            p++;

        uint32_t len = p - start;
        dentry_t *child = current->children;
        int found = 0;

        while (child)
        {
            if (match_seg(child->name, start, len))
            {
                current = child;
                found = 1;
                break;
            }
            child = child->next;
        }
        if (!found)
            return 0;
    }

    return current;
}

int sys_open(const char *path, uint32_t flags)
{
    if (!path)
        return -1;
    dentry_t *dentry = vfs_lookup(vfs_root, path);

    if (!dentry || !dentry->inode)
        return -1;
    inode_t *inode = dentry->inode;

    file_t *file = kmalloc(sizeof(file_t));

    if (!file)
        return -1;

    file->inode = inode;
    file->offset = 0;
    file->flags = flags;

    for (int i = 0; i < 32; i++)
    {
        if (!current->fd_table[i])
        {
            current->fd_table[i] = file;
            return i;
        }
    }
    kfree(file);
    return -1;
}