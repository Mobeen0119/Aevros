#include "Syscall_U.h"

int write(int fd, char *buff, int size)
{
    return syscall(SYS_WRITE, fd, (int)buff, size);
}