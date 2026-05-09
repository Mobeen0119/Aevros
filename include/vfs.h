
#ifndef VFS_H
#define VFS_H
#include <stdint.h>

struct vfs_node;

//--------ptr to fnc--ptr to file_node--offset(start)--size--buffer

typedef uint32_t (*read_fn)(vfs_node_t *, uint32_t, uint32_t, uint8_t *);
typedef uint32_t (*write_fn)(vfs_node_t *, uint32_t, uint32_t, uint8_t *);

typedef struct vfs_ops
{
    read_fn read;
    write_fn write;
} vfs_ops_t;

#define VFS_FILE 1
#define VFS_DIR 2
#define VFS_DEVICE 4

typedef struct vfs_node vfs_node_t;

typedef struct file_descriptor
{
    vfs_node_t *node;
    uint32_t position, flags;
} fd_t;

typedef struct vfs_node
{
    char name[128];
    uint32_t flags, length;
    struct vfs_node *parent;
    struct vfs_node **children;
    uint32_t child_count;
    uint32_t gid;

    vfs_ops_t *ops;
    void *fs_private; // backend private data

} vfs_node_t;

#endif