#include <stdint.h>
#include "../Memory/kheap.c"
#include "../Process/task.h"
#include "..\include\vfs.h"
#include "../include/RAMFS.h"
#include "../LIB/string.c"
#define READ_ONLY 0x01
#define WRITE_ONLY 0x02
#define READ_WRITE 3
#define CREAT 0x04

dentry_t *vfs_root = 0;
static devfs_device_t devfs_table[MAX_DEVICES];
static int devfs_count = 0;

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
    int last = -1;
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

dentry_t *vfs_lookup(dentry_t *root, const char *path)
{

    if (!root || !path)
        return 0;

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
        dentry_t *child = dir->children;
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
            return 0;

        while (child)
        {
            if (match_seg(child->name, start, len))
            {
                dir = child;
                found = 1;
                break;
            }

            child = child->next;
        }
        if (!found)
            return 0;

        dir = found;
        if (child->mount)
            dir = dir->mount->root;
    }

    return dir;
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

            if (!parent || !(parent->inode->flags & VFS_DIR))
                return -1;

            dentry = ramfs_create_files(parent, name);
            if (!dentry)
                return -1;
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
        if (!current_task->fd_table[i])
        {
            current_task->fd_table[i] = file;
            inode->ref_count++;
            return i;
        }
    }
    kfree(file);
    return -1;
}

int sys_read(int fd, uint8_t *buf, uint32_t size)
{
    if (fd < 0 || fd >= 32 || !current_task->fd_table[fd])
    {
        return -1;
    }

    file_t *file = current_task->fd_table[fd];

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
    if (fd < 0 || fd >= 32 || !current_task->fd_table[fd])
        return -1;

    file_t *file = current_task->fd_table[fd];

    if (!file || !file->inode || !file->inode->ops || !file->inode->ops->write)
    {
        return -1;
    }

    if (!(file->flags & WRITE_ONLY) && !(file->flags & READ_WRITE))
        return -1;

    int byte_write = file->inode->ops->write(file->inode, file->offset, size, buf);

    if (byte_write > 0)
        file->offset += byte_write;
    return byte_write;
}

int sys_close(int fd)
{
    if (fd < 0 || fd >= 32)
        return -1;

    file_t *file = current_task->fd_table[fd];

    if (!file)
        return -1;

    inode_t *inode = file->inode;

    if (inode)
    {
        if (inode->ref_count > 0)
            inode->ref_count--;
    }
    kfree(file);
    current_task->fd_table[fd] = 0;

    return 0;
}

int sys_mkdir(const char *path)
{
    if (!path)
        return -1;

    char parent_path[256];
    parent_dirname(path, parent_path);

    char *name = basename(path);

    dentry_t *parent = vfs_lookup(vfs_root, parent_path);

    if (!parent)
        return 0;

    if (!(parent->inode->flags & VFS_DIR))
        return -1;

    dentry_t *exist = vfs_lookup(vfs_root, path);
    if (exist)
        return -1;

    dentry_t *dir = ramfs_mkdir(parent, name);

    if (!dir)
        return -1;

    return 0;
}

int sys_unlink(const char *path)
{
    if (!path)
        return -1;

    dentry_t *target = vfs_lookup(vfs_root, path);
    if (!target)
        return -1;

    if (target == vfs_root)
        return -1;

    dentry_t *parent = target->parent;
    inode_t *inode = target->inode;

    if (!parent || !inode)
        return -1;

    dentry_t *prev = 0, *current = parent->children;

    if ((inode->flags & VFS_DIR) && target->children)
        return -1;
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

    return 0;
}

int sys_chdir(const char *path)
{
    if (!path)
        return -1;

    dentry_t *dir = vfs_lookup(vfs_root, path);
    if (!dir)
        return -1;

    if (!(dir->inode->flags & VFS_DIR))
        return -1;

    current_task->cwd = dir;

    return 0;
}

int vfs_mount(dentry_t *mount_point, dentry_t *root)
{
    if (!mount_point || !root)
        return -1;

    if (!(mount_point->inode->flags & VFS_DIR))
        return -1;

    vfs_mount_t *mnt = kmalloc(sizeof(vfs_mount_t));

    if (!mnt)
        return -1;
    mnt->root = root;
    mnt->flags = 0;

    mount_point->mount = mnt;

    return 0;
}

int devfs_register(const char *name, inode_t *inode)
{
    if (!name || !inode)
        return -1;

    if (devfs_count >= MAX_DEVICES)
        return -1;

    for (int i = 0; i < devfs_count; i++)
    {
        if (match_seg(devfs_table[i].name, name, strlen(name)))
            return -1;
    }
    devfs_table[devfs_count].name = name;
    devfs_table[devfs_count].inode = inode;

    devfs_count++;

    return 0;
}

inode_t *devfs_get(const char *name)
{
    if (!name)
        return -1;

    for (int i = 0; i < devfs_count; i++)
    {
        if (devfs_table[devfs_count].name, name, strlen(name))
            return devfs_table[devfs_count].inode;
    }
    return 0;
}
