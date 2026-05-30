#ifndef SYSCALL_H
#define SYSCALL_H
#include "../CPU/idt.h"
#include "../Paging/isr.h"
#define SYS_READ   0
#define SYS_WRITE  1
#define SYS_OPEN   2
#define SYS_CLOSE  3
#define SYS_FORK   4
#define SYS_EXIT   5

int syscall_dispatcher(int num,int arg1,int arg2,int arg3);
static inline int syscall(int num,int arg1,int arg2,int arg3);

void syscall_handler(register_t *regs);

#endif