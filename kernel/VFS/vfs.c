#include <stdint.h>
#include "../Memory/kheap.h"
#include "../Process/task.h"
#include "../../Include/vfs.h"
#include "../../Include/ramfs.h"
#include "../../LIB/string.h"
#include "../Dev/dev.h"
#include "../Memory/pmm.h"
#include "../Process/TaskLife/tasklife.h"


dentry_t *vfs_root = 0;

void vfs_init()
{
    vfs_root = (dentry_t *)kmalloc_raw(sizeof(dentry_t));
    if (!vfs_root)
    {
        volatile char *v = (volatile char *)0xB8000;
        v[20] = 'X';
        v[21] = 0x0C;
        while (1)
            asm volatile("hlt");
    }
    memset(vfs_root, 0, sizeof(dentry_t));
    vfs_root->name = "/";
    vfs_root->parent = vfs_root;
    vfs_root->inode = (inode_t *)kmalloc_raw(sizeof(inode_t));
    memset(vfs_root->inode, 0, sizeof(inode_t));
    vfs_root->inode->flags = VFS_DIR;
}

uint32_t vfs_read(dentry_t *node, uint32_t offset, uint32_t size, uint8_t *buffer)
{
    if (node && node->inode && node->inode->ops && node->inode->ops->read)
    {
        return node->inode->ops->read(node, offset, size, buffer);
    }
    return VFS_ERR;
}

uint32_t vfs_write(dentry_t *node, uint32_t offset, uint32_t size, uint8_t *buffer)
{
    if (node && node->inode && node->inode->ops && node->inode->ops->write)
    {
        return node->inode->ops->write(node, offset, size, buffer);
    }
    return VFS_ERR;
}

static const char *skip_slash(const char *p)
{
    while (*p == '/')
        p++;
    return p;
}


int match_seg(const char *name, const char *start, uint32_t len)
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
        return NULL;

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
            return NULL;

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
            return NULL;
    }

    return dir;
}

dentry_t *vfs_follow_mount(dentry_t *dentry)
{
    if (!dentry)
        return NULL;
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

            const char *name = basename(path);
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

    file_t *file = kmalloc_raw(sizeof(file_t));

    if (!file)
        return VFS_ERR;

    file->inode = inode;
    file->dentry = vfs_follow_mount(dentry);
    file->offset = 0;
    file->flags = flags;

    for (int i = 0; i < TASK_MAX_FDS; i++)
    {
        if (!current_task->fd_table[i])
        {
            current_task->fd_table[i] = file;
            inode->ref_count++;
            return i;
        }
    }
    task_log_event(current_task, EVT_FD_OPEN, (uint32_t)file);

    task_log_event(current_task, EVT_FD_CLOSE, (uint32_t)file);
    kfree_raw(file);
    return VFS_ERR;
}

int sys_read(int fd, uint8_t *buf, uint32_t size)
{
    if (fd < 0 || fd >= TASK_MAX_FDS || !current_task->fd_table[fd])
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

    int bytes_read = file->inode->ops->read(file->dentry, file->offset, size, buf);
    if (bytes_read > 0)
        file->offset += bytes_read;

    return bytes_read;
}

int sys_write(int fd, uint8_t *buf, uint32_t size)
{
    if (fd < 0 || fd >= TASK_MAX_FDS || !current_task->fd_table[fd])
        return VFS_ERR;

    file_t *file = current_task->fd_table[fd];

    if (!file || !file->inode || !file->inode->ops || !file->inode->ops->write)
    {
        return VFS_ERR;
    }

    if (!(file->flags & WRITE_ONLY) && !(file->flags & READ_WRITE))
        return VFS_ERR;

    int byte_write = file->inode->ops->write(file->dentry, file->offset, size, buf);

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
    kfree_raw(file);
    current_task->fd_table[fd] = 0;

    return VFS_OK;
}

int sys_mkdir(const char *path)
{
    if (!path)
        return VFS_ERR;

    char parent_path[256];
    parent_dirname(path, parent_path);

    const char *name = basename(path);

    dentry_t *parent = vfs_lookup(vfs_root, parent_path);

    if (!parent)
        return VFS_ERR;

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

    uint32_t bucket = dentry_hash(target->name);
    dentry_t *current = parent->hash_bucket[bucket];
    dentry_t *prev = NULL;

    if ((inode->flags & VFS_DIR) && target->children)
        return VFS_ERR;

    while (current)
    {
        if (current == target)
        {
            if (!prev)
            {
                parent->hash_bucket[bucket] = current->hash_next;
            }
            else
                prev->hash_next = current->hash_next;
            break;
        }
        prev = current;
        current = current->hash_next;
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
                        kfree_raw(ram->data);

                    kfree_raw(ram);
                }
            }
            kfree_raw(inode);
        }
    }

    if (target->name)
        kfree_raw(target->name);
    kfree_raw(target);

    return VFS_OK;
}

int sys_chdir(const char *path)
{
    if (!path)
        return VFS_ERR;

    dentry_t *target_dir = vfs_lookup(vfs_root, path);
    if (!target_dir)
        return VFS_ERR; 

    if (!(target_dir->inode->flags & VFS_DIR))
        return VFS_ERR;

    current_task->cwd = target_dir; 

    return VFS_OK;
}

int vfs_mount(dentry_t *mount_point, dentry_t *root)
{
    if (!mount_point || !root)
        return VFS_ERR;

    if (!(mount_point->inode->flags & VFS_DIR))
        return VFS_ERR;

    vfs_mount_t *mnt = kmalloc_raw(sizeof(vfs_mount_t));

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

    dentry_t *dir = file->dentry;

    if (!dir)
        return VFS_ERR;

    uint32_t count = 0;

    for (uint32_t b = 0; b < DENTRY_HASH; b++)
    {
        dentry_t *child = dir->hash_bucket[b];

        while (child)
        {
            if (count == file->offset)
            {
                strcpy(dirent->name, child->name);
                dirent->type = child->inode->flags;

                file->offset++;
                return 1;
            }
            count++;
            child = child->hash_next;
        }
    }

    return 0;
}
