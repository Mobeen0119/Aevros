#include "aevros.h"

static int sys3(int num, int a1, int a2, int a3)
{
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(num), "b"(a1), "c"(a2), "d"(a3) : "memory");
    return ret;
}

#define SYS_WRITE_NUM 1
#define SYS_READ_NUM 2
#define SYS_OPEN_NUM 3
#define SYS_CLOSE_NUM 4
#define SYS_EXIT_NUM 6

int write(int fd, const char *buf, int size)
{
    return sys3(SYS_WRITE_NUM, fd, (int)buf, size);
}

int read(int fd, char *buf, int size)
{
    return sys3(SYS_READ_NUM, fd, (int)buf, size);
}

int open(const char *path, int flags)
{
    return sys3(SYS_OPEN_NUM, (int)path, flags, 0);
}

int close(int fd)
{
    return sys3(SYS_CLOSE_NUM, fd, 0, 0);
}

void exit(int code)
{
    sys3(SYS_EXIT_NUM, code, 0, 0);
    while (1)
        ;
}

int strlen(const char *s)
{
    int n = 0;
    while (s[n])
        n++;
    return n;
}

void puts(const char *s)
{
    write(AEV_STDOUT, s, strlen(s));
}

void putnum(int n)
{
    char buf[12];
    int i = 11;
    int neg = n < 0;
    unsigned int u = neg ? (unsigned int)(-n) : (unsigned int)n;

    buf[i] = 0;
    do
    {
        buf[--i] = (char)('0' + u % 10);
        u /= 10;
    } while (u);
    if (neg)
        buf[--i] = '-';
    puts(&buf[i]);
}