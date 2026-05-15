
#ifndef VFS_H
#define VFS_H
#include <stdint.h>
#define VFS_FILE 1
#define VFS_DIR 2
#define VFS_DEVICE 4
#define MAX_DEVICES 64
#define VFS_OK 0
#define VFS_ERR -1
#define VFS_ENOENT -2
#define VFS_EACCES -3
#define VFS_ENOTDIR -4
#define VFS_ENOMEM -5
#define VFS_EEXIST -6
#define DENTRY_HASH 32

typedef struct dentry dentry_t;

//--------ptr to fnc-->ptr to file_node-->offset(start)-->size-->buffer

typedef uint32_t (*read_fn)(dentry_t *, uint32_t, uint32_t, uint8_t *);
typedef uint32_t (*write_fn)(dentry_t *, uint32_t, uint32_t, uint8_t *);

typedef struct vfs_ops
{
    read_fn read;
    write_fn write;

} vfs_ops_t;

typedef struct dirent
{
    char *name;
    uint32_t type;
} dirent_t;

typedef struct vfs_node vfs_node_t;
struct inode;

typedef struct file_descriptor // Open file state
{
    struct inode *inode;
    uint32_t position, flags, offset;

} file_t;

typedef struct inode
{ // File metadata
    uint32_t size, flags, ref_count;
    vfs_ops_t *ops;
    void *fs_private;
    dentry_t *dentry;
} inode_t;

typedef struct vfs_mount
{
    dentry_t *root;
    uint32_t flags;
} vfs_mount_t;

typedef struct dentry // name Cache layer
{
    char *name;
    inode_t *inode;
    struct dentry *parent;
    struct dentry *children;
    struct vfs_mount *mount;
    struct dentry *next; // bucket chain
    struct dentry *hash_bucket[DENTRY_HASH];
    struct dentry *hash_next;

} dentry_t;


uint32_t dentry_hash(const char *name);
int sys_readdir(int fd, dirent_t *dirent);

#endif