#include "tty.h"
#include "../Include/screen.h"
#include "keyboard.h"
#include "../kernel/Memory/kheap.h"
#include "../kernel/Dev/dev.h"
#include "../kernel/Memory/pmm.h"

static vfs_ops_t tty_ops = {
    .read = (read_fn)tty_read,
    .write = (write_fn)tty_write};

void tty_init()
{
    inode_t *tty_inode = (inode_t *)kmalloc(sizeof(inode_t));
    memset(tty_inode, 0, sizeof(inode_t));

    tty_inode->flags = VFS_DEVICE;
    tty_inode->ops = &tty_ops;
    devfs_register("tty", tty_inode);
}

int tty_write(dentry_t *dentry, uint32_t offset, uint32_t size, uint8_t *buffer)
{

    (void)dentry;
    (void)offset;

    if (!buffer)
        return VFS_ERR;

    for (uint32_t i = 0; i < size; i++)
    {
        kput_char((char)buffer[i]);
    }

    return size;
}

int tty_read(dentry_t *dentry, uint32_t offset, uint32_t size, uint8_t *buffer)
{

    (void)dentry;
    (void)offset;

    if (!buffer)
        return 0;

    for (uint32_t i = 0; i < size; i++)
    {
        buffer[i] = keyboard_getchar();

        if (buffer[i] == '\n')
            return i + 1;
    }

    return size;
}