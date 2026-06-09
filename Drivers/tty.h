#ifndef TTY_H
#define TTY_H
#include<stdint.h>
#include "../Include/vfs.h"


void tty_init();

int tty_write(dentry_t *dentry, uint32_t offset, uint32_t size, uint8_t *buffer);
int tty_read(dentry_t *dentry, uint32_t offset, uint32_t size, uint8_t *buffer);

#endif