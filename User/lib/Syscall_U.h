#pragma once

enum
{

    SYS_READ = 0,
    SYS_WRITE,
    SYS_OPEN,
    S_CLOSE,
    SYS_EXIT,
};

int syscall(int num, int a1, int a2, int a3);
int write(int fd, char *buf, int size);
int read(int fd, char *buf, int size);

void exit();