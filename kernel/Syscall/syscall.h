#ifndef SYSCALL_H
#define SYSCALL_H
#include "../CPU/idt.h"
#include "../Paging/isr.h"
#define SYS_WRITE  1
#define SYS_READ   2
#define SYS_OPEN   3
#define SYS_CLOSE  4
#define SYS_FORK   5
#define SYS_EXIT   6
#define SYS_WAITPID 7
#define SYS_EXEC 8


static inline int syscall(int num,int arg1,int arg2,int arg3);

void syscall_handler(register_t *regs);

void init_syscalls();

int sys_fork();

#endif