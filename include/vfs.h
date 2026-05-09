
#ifndef VFS_H
#define VFS_H
#include <stdint.h>
#define VFS_FILE 1
#define VFS_DIR 2
#define VFS_DEVICE 4

typedef struct vfs_node vfs_node_t;

//--------ptr to fnc-->ptr to file_node-->offset(start)-->size-->buffer

typedef uint32_t (*read_fn)(vfs_node_t *, uint32_t, uint32_t, uint8_t *);
typedef uint32_t (*write_fn)(vfs_node_t *, uint32_t, uint32_t, uint8_t *);

typedef struct vfs_ops
{
    read_fn read;
    write_fn write;

} vfs_ops_t;

typedef struct vfs_node vfs_node_t;
struct inode;

typedef struct file_descriptor // Open file state
{
    struct inode *inode;
    uint32_t position, flags,offset;

} file_t;

typedef struct inode
{ // File metadata
    uint32_t size, flags;
    vfs_ops_t *ops;
    void *fs_private;
} inode_t;

typedef struct dentry
{                             // name Cache layer
    char *name;
    inode_t *inode;
    dentry_t *parent;
    dentry_t *children;
    struct dentry *next; // hash bucket
} dentry_t;

#endif