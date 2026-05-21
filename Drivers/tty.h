#ifndef TTY_H
#define TTY_H
#include<stdint.h>
#include "../Include/vfs.h"


int tty_write(inode_t* inode,uint32_t offset,uint32_t size,uint8_t* buffer);



#endif