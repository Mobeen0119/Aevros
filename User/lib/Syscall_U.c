#include "Syscall_U.h"

int syscall(int num, int a1, int a2, int a3)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret)
                 : "a"(num),
                   "b"(a1),
                   "c"(a2),
                   "d"(a3));

    return ret;
}