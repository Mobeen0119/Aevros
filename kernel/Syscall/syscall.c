#include "syscall.h"
#include "../../Include/vfs.h"
#include "../Process/task.h"
#include "../CPU/idt.h"
#include "../Paging/isr.h"
#include "../Process/fork.c"

extern void isr128();

void init_syscalls()
{
    idt_gate_set(128, (uint32_t)isr128, 0xEE);
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

void syscall_handler(register_t *regs)
{
    uint32_t num = regs->eax;
    uint32_t a1 = regs->ebx;
    uint32_t a2 = regs->ecx;
    uint32_t a3 = regs->edx;

    uint32_t res = -1;

    switch (num)
    {
    case SYS_WRITE:
        res = sys_write(a1, (uint8_t *)a2, a3);
        break;

    case SYS_READ:
        res = sys_read(a1, (uint8_t *)a2, a3);
        break;

    case SYS_OPEN:
        res = sys_open((char *)a1, a2);
        break;

    case SYS_CLOSE:
        res = sys_close(a1);
        break;

    case SYS_FORK:
        res = do_fork();
        break;

    case SYS_EXIT:
        sys_exit(0);
        res = 0;
        break;

    default:
        res = (uint32_t)-1;
        break;
    }
    regs->eax = res;
}

int sys_fork()
{
    return do_fork();
}
