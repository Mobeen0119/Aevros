#include "syscall.h"
#include "../../Include/vfs.h"
#include "../Process/task.h"
#include "../../Include/idt.h"


extern void isr128();

void init_syscalls(){
    idt_gate_set(128,(uint32_t)isr128,0xEE);
}

static inline int syscall(int num, int arg1, int arg2, int arg3)
{
    int ret;

    asm volatile(
        "int $0x80" : "=a"(ret) : "a"(num),   // EAX
                                  "b"(arg1),  // EBX
                                  "c"(arg2),  // ECX
                                  "d"(arg3)); // EDX
    return ret;
}

int syscall_dispatcher(int num, int arg1, int arg2, int arg3)
{

    switch (num)
    {
    case SYS_READ:
        return sys_read(arg1, (uint8_t *)arg2, arg3);

    case SYS_WRITE:
        return sys_write(arg1, (uint8_t *)arg2, arg3);

    case SYS_OPEN:
        return sys_open((char *)arg1, arg2);

    case SYS_CLOSE:
        return sys_close(arg1);

    case SYS_EXIT:
        return sys_exit();
        return 0;

    default:
        return -1;
    }
}

int syscall
