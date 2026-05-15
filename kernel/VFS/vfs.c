#include <stdint.h>
#include "../Memory/kheap.c"
#include "../Process/task.h"
#include "..\include\vfs.h"
#include "../include/RAMFS.h"
#include "../LIB/string.c"
#include "../Dev/dev.h"
#define READ_ONLY 0x01
#define WRITE_ONLY 0x02
#define READ_WRITE 0x03
#define CREAT 0x04

dentry_t *vfs_root = 0;


uint32_t vfs_read(dentry_t *node, uint32_t offset, uint32_t size, uint8_t *buffer)
{
    if (node && node->inode && node->inode->ops && node->inode->ops->read)
    {
        return node->inode->ops->read(node->inode, offset, size, buffer);
    }
    return VFS_OK;
}

uint32_t vfs_write(dentry_t *node, uint32_t offset, uint32_t size, uint8_t *buffer)
{
    if (node && node->inode && node->inode->ops && node->inode->ops->write)
    {
        return node->inode->ops->write(node->inode, offset, size, buffer);
    }
    return VFS_OK;
}

static const char *skip_slash(const char *p)
{
    while (*p == '/')
        p++;
    return p;
}

//-----------------------a => what exist ---b => what asked for

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

uint32_t dentry_hash(const char *name)
{
    uint32_t hash = 0;
    while (*name)
    {
        hash = hash * 31 + *name;
        name++;
    }
    return hash % DENTRY_HASH;
}

const char *basename(const char *path)
{
    const char *last = path;

    while (*path)
    {
        if (*path == '/')
            last = path + 1;
        path++;
    }
    return last;
}

void parent_dirname(const char *path, char *parent)
{
    int last = VFS_ERR;
    for (int i = 0; path[i]; i++)
    {
        if (path[i] == '/')
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

dentry_t *vfs_resolve_path(dentry_t *root, const char *path)
{

    if (!root || !path)
        return VFS_OK;

    const char *p = path;
    dentry_t *dir;

    if (p[0] == '/')
        dir = root;
    else
        dir = current_task->cwd;

    while (*p)
    {
        p = skip_slash(p);

        if (!*p)
            break;

        const char *start = p;

        while (*p && *p != '/')
            p++;

        uint32_t len = p - start;
        char temp[256];
        memcpy(temp, start, len);
        temp[len] = '\0';

        uint32_t bucket = dentry_hash(temp);
        dentry_t *child = dir->hash_bucket[bucket];
        int found = 0;

        if (match_seg(".", start, len))
        {
            continue;
        }
        if (match_seg("..", start, len))
        {
            if (dir->parent)
                dir = dir->parent;
            continue;
        }

        if (!(dir->inode->flags & VFS_DIR))
            return VFS_OK;

        while (child)
        {
            if (match_seg(child->name, start, len))
            {
                dir = child;
                found = 1;
                break;
            }
            child = child->hash_next;
        }
        if (!found)
            return VFS_OK;
    }

    return dir;
}

dentry_t *vfs_follow_mount(dentry_t *dentry)
{
    if (!dentry)
        return VFS_OK;
    if (dentry->mount)
        return dentry->mount->root;

    return dentry;
}

dentry_t *vfs_lookup(dentry_t *root, const char *path)
{
    dentry_t *node = vfs_resolve_path(root, path);
    return vfs_follow_mount(node);
}

int sys_open(const char *path, uint32_t flags)
{
    if (!path)
        return VFS_ERR;
    dentry_t *dentry = vfs_lookup(vfs_root, path);

    if (!dentry)
    {
        if (flags & CREAT)
        {
            char parent_path[256];

            parent_dirname(path, parent_path);

            char *name = basename(path);
            dentry_t *parent = vfs_lookup(vfs_root, parent_path);

            if (!parent || !(parent->inode->flags & VFS_DIR))
                return VFS_ERR;

            dentry = ramfs_create_files(parent, name);
            if (!dentry)
                return VFS_ERR;
        }
        else
            return VFS_ERR;
    }

    inode_t *inode = dentry->inode;

    file_t *file = kmalloc(sizeof(file_t));

    if (!file)
        return VFS_ERR;

    file->inode = inode;
    file->offset = 0;
    file->flags = flags;

    for (int i = 0; i < 32; i++)
    {
        if (!current_task->fd_table[i])
        {
            current_task->fd_table[i] = file;
            inode->ref_count++;
            return i;
        }
    }
    kfree(file);
    return VFS_ERR;
}

int sys_read(int fd, uint8_t *buf, uint32_t size)
{
    if (fd < 0 || fd >= 32 || !current_task->fd_table[fd])
    {
        return VFS_ERR;
    }

    file_t *file = current_task->fd_table[fd];

    if (!file || !file->inode || !file->inode->ops || !file->inode->ops->read)
    {
        return VFS_ERR;
    }

    if (!(file->flags & READ_ONLY) && !(file->flags & READ_WRITE))
        return VFS_ERR;

    int bytes_read = file->inode->ops->read(file->inode, file->offset, size, buf);
    if (bytes_read > 0)
        file->offset += bytes_read;

    return bytes_read;
}

int sys_write(int fd, uint8_t *buf, uint32_t size)
{
    if (fd < 0 || fd >= 32 || !current_task->fd_table[fd])
        return VFS_ERR;

    file_t *file = current_task->fd_table[fd];

    if (!file || !file->inode || !file->inode->ops || !file->inode->ops->write)
    {
        return VFS_ERR;
    }

    if (!(file->flags & WRITE_ONLY) && !(file->flags & READ_WRITE))
        return VFS_ERR;

    int byte_write = file->inode->ops->write(file->inode, file->offset, size, buf);

    if (byte_write > 0)
        file->offset += byte_write;
    return byte_write;
}

int sys_close(int fd)
{
    if (fd < 0 || fd >= 32)
        return VFS_ERR;

    file_t *file = current_task->fd_table[fd];

    if (!file)
        return VFS_ERR;

    inode_t *inode = file->inode;

    if (inode)
    {
        if (inode->ref_count > 0)
            inode->ref_count--;
    }
    kfree(file);
    current_task->fd_table[fd] = 0;

    return VFS_OK;
}

int sys_mkdir(const char *path)
{
    if (!path)
        return VFS_ERR;

    char parent_path[256];
    parent_dirname(path, parent_path);

    char *name = basename(path);

    dentry_t *parent = vfs_lookup(vfs_root, parent_path);

    if (!parent)
        return VFS_OK;

    if (!(parent->inode->flags & VFS_DIR))
        return VFS_ERR;

    dentry_t *exist = vfs_lookup(vfs_root, path);
    if (exist)
        return VFS_ERR;

    dentry_t *dir = ramfs_mkdir(parent, name);

    if (!dir)
        return VFS_ERR;

    return VFS_OK;
}

int sys_unlink(const char *path)
{
    if (!path)
        return VFS_ERR;

    dentry_t *target = vfs_lookup(vfs_root, path);
    if (!target)
        return VFS_ERR;

    if (target == vfs_root)
        return VFS_ERR;

    dentry_t *parent = target->parent;
    inode_t *inode = target->inode;

    if (!parent || !inode)
        return VFS_ERR;

    dentry_t *prev = 0, *current = parent->children;

    if ((inode->flags & VFS_DIR) && target->children)
        return VFS_ERR;
    while (current)
    {
        if (current == target)
        {
            if (!prev)
            {
                parent->children = current->next;
            }
            else
                prev->next = current->next;
            break;
        }
        prev = current;
        current = current->next;
    }

    if (inode)
    {
        if (inode->ref_count > 0)
            inode->ref_count--;
        if (inode->ref_count == 0)
        {
            if (inode->flags & VFS_FILE)
            {
                ramfs_inode_t *ram = (ramfs_inode_t *)inode->fs_private;
                if (ram)
                {
                    if (ram->data)
                        kfree(ram->data);

                    kfree(ram);
                }
            }
            kfree(inode);
        }
    }

    if (target->name)
        kfree(target->name);
    kfree(target);

    return VFS_OK;
}

int sys_chdir(const char *path)
{
    if (!path)
        return VFS_ERR;

    dentry_t *dir = vfs_lookup(vfs_root, path);
    if (!dir)
        return VFS_ERR;

    if (!(dir->inode->flags & VFS_DIR))
        return VFS_ERR;

    current_task->cwd = dir;

    return VFS_OK;
}

int vfs_mount(dentry_t *mount_point, dentry_t *root)
{
    if (!mount_point || !root)
        return VFS_ERR;

    if (!(mount_point->inode->flags & VFS_DIR))
        return VFS_ERR;

    vfs_mount_t *mnt = kmalloc(sizeof(vfs_mount_t));

    if (!mnt)
        return VFS_ERR;
    mnt->root = root;
    mnt->flags = 0;

    mount_point->mount = mnt;

    return VFS_OK;
}

int sys_readdir(int fd, dirent_t *dirent)
{
    if (fd < 0 || fd >= 32)
        return VFS_ERR;

    file_t *file = current_task->fd_table[fd];

    if (!file || !file->inode)
    {
        return VFS_ERR;
    }

    if (!(file->inode->flags & VFS_DIR))
        return VFS_ERR;

    dentry_t *dir = file->inode->dentry;

    if (!dir)
        return VFS_ERR;

    dentry_t *child = dir->children;
    uint32_t index = 0;

    while (child && index < file->offset)
    {
        child = child->next;
        index++;
    }

    if (!child)
        return 0;

    strcpy(dirent->name, child->name);
    dirent->type = child->inode->flags;

    file->offset++;

    return 1;
}
