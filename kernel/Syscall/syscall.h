#ifndef SYSCALL_H
#define SYSCALL_H
#define SYS_READ   0
#define SYS_WRITE  1
#define SYS_OPEN   2
#define SYS_CLOSE  3
#define SYS_EXIT   4

int syscall_dispatcher(int num,int arg1,int arg2,int arg3);
static inline int syscall(int num,int arg1,int arg2,int arg3);

#endif