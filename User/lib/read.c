#include "Syscall_U.h"

int read(int fd, char *buff, int size)
{
    return syscall(SYS_READ, fd, (int)buff, size);
}