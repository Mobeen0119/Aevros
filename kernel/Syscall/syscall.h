#ifndef SYSCALL_H
#define SYSCALL_H
#include "../CPU/idt.h"
#include "../Paging/isr.h"

#define SYS_WRITE   1
#define SYS_READ    2
#define SYS_OPEN    3
#define SYS_CLOSE   4
#define SYS_FORK    5
#define SYS_EXIT    6
#define SYS_WAITPID 7
#define SYS_EXEC    8
#define SYS_SOCKET 9
#define SYS_BIND   10
#define SYS_CONNECT  11
#define SYS_ACCEPT 12
#define SYS_SEND 13
#define SYS_RECV  14
#define SYS_SENDTO 15
#define SYS_RECVFROM 16
#define SYS_SOCKCLOSE 17

int syscall(int num, int arg1, int arg2, int arg3);

void syscall_handler(register_t *regs);
void init_syscalls(void);
void sys_print(char *user_string);
int sys_fork(register_t *regs);

#endif