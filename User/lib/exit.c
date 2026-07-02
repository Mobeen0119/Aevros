#include "Syscall_U.h"

void exit()
{
    syscall(SYS_EXIT, 0, 0, 0);
    while(1);
}