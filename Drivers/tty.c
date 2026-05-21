#include "tty.h"
#include "../Include/screen.h"
#include "keyboard.h"

int tty_write(inode_t *inode, uint32_t offset, uint32_t size, uint8_t *buffer)
{

    (void)inode;
    (void)offset;

    if(!buffer) return VFS_ERR;

    for (uint32_t i = 0; i < size; i++)
    {
        kput_char((char)buffer[i]);
    }

    return size;
}

int tty_read(inode_t *inode, uint32_t offset, uint32_t size, uint8_t *buffer)
{

    (void)inode;
    (void)offset;

    if(!buffer) return VFS_ERR;

    for (uint32_t i = 0; i < size; i++)
    {
        buffer[i]=keyboard_getchar();

        if(buffer[i]=='\n') return i+1;
    }

    return size;
}