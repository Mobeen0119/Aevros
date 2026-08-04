#pragma once

enum
{

    SYS_WRITE = 1,
    SYS_READ = 2,
    SYS_OPEN = 3,
    SYS_CLOSE = 4,
    SYS_FORK = 5,
    SYS_EXIT = 6,
    SYS_WAITPID = 7,
    SYS_EXEC = 8,
    SYS_SOCKET = 9,
    SYS_BIND = 10,
    SYS_CONNECT = 11,
    SYS_ACCEPT = 12,
    SYS_SEND = 13,
    SYS_RECV = 14,
    SYS_SENDTO = 15,
    SYS_RECVFROM = 16,
    SYS_SOCKCLOSE = 17

};

int syscall(int num, int a1, int a2, int a3);
int write(int fd, char *buf, int size);
int read(int fd, char *buf, int size);

void exit();