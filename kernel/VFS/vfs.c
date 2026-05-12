#include <stdint.h>
#include "../Memory/kheap.c"
#include "../Process/task.h"
#include "..\include\vfs.h"
#include "../include/RAMFS.h"
#define READ_ONLY 0x01
#define WRITE_ONLY 0x02
#define READ_WRITE (READ_ONLY | WRITE_ONLY)
#define CREAT 0x04

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

    if (!dentry)
    {
        if (flags & CREAT)
        {
            char parent_path[256];

            parent_dirname(path, parent_path);

            char *name = basename(path);
            dentry_t *parent = vfs_lookup(vfs_root, parent_path);

            if (!parent)
                return -1;

            dentry = ramfs_create_files(parent, name);
        }
        else
            return -1;
    }

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
            inode->ref_count++;
            return i;
        }
    }
    kfree(file);
    return -1;
}

int sys_read(int fd, uint8_t *buf, uint32_t size)
{
    if (fd < 0 || fd >= 32 || !current->fd_table[fd])
    {
        return -1;
    }

    file_t *file = current->fd_table[fd];

    if (!file || !file->inode || !file->inode->ops || !file->inode->ops->read)
    {
        return -1;
    }

    if (!(file->flags & READ_ONLY) && !(file->flags & READ_WRITE))
        return -1;

    int bytes_read = file->inode->ops->read(file->inode, file->offset, size, buf);
    if (bytes_read > 0)
        file->offset += bytes_read;

    return bytes_read;
}

int sys_write(int fd, uint8_t *buf, uint32_t size)
{
    if (fd < 0 || fd >= 32 || !current->fd_table[fd])
        return -1;

    file_t *file = current->fd_table[fd];

    if (!file || !file->inode || !file->inode->ops || !file->inode->ops->write)
    {
        return -1;
    }

    if (!(file->flags & WRITE_ONLY) && !(file->flags & READ_WRITE))
    {
        return -1;
    }

    int byte_write = file->inode->ops->write(file->inode, file->offset, size, buf);

    if (byte_write > 0)
        file->offset += byte_write;
    return byte_write;
}

int sys_close(int fd)
{
    if (fd < 0 || fd >= 32)
        return -1;

    file_t *file = current->fd_table[fd];

    if (!file)
        return -1;

    inode_t *inode = file->inode;

    if (inode)
    {
        if (inode->ref_count > 0)
            inode->ref_count--;
    }
    kfree(file);
    current->fd_table[fd] = 0;

    return 0;
}
