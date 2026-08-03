#include "syscall.h"
#include "../../Include/vfs.h"
#include "../Process/task.h"
#include "../CPU/idt.h"
#include "../Paging/isr.h"
#include "../../Include/screen.h"
#include "../Process/exec.h"
#include "../../Networking/FrontDoor/frontdoor.h"

int syscall(int num, int arg1, int arg2, int arg3)
{
    int ret;
    asm volatile(
        "int $0x80" : "=a"(ret) : "a"(num),
                                  "b"(arg1),
                                  "c"(arg2),
                                  "d"(arg3));
    return ret;
}

void syscall_handler(register_t *regs)
{
    uint32_t num = regs->eax;
    uint32_t a1 = regs->ebx;
    uint32_t a2 = regs->ecx;
    uint32_t a3 = regs->edx;

    uint32_t res = (uint32_t)-1;

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
        res = do_fork(regs);
        break;

    case SYS_EXIT:
        sys_exit(a1);
        break;

    case SYS_WAITPID:
        res = sys_waitpid(a1, (int *)a2);
        break;

    case SYS_EXEC:
        res = sys_exec((const char *)a1);
        break;

    case SYS_SOCKET:
        res = (uint32_t)frontdoor_socket((int)a1);
        break;

    case SYS_BIND:
        res = (uint32_t)frontdoor_bind((int)a1, (uint16_t)a2);
        break;

    case SYS_CONNECT:
        res = (uint32_t)frontdoor_connect((int)a1, (const sock_addr_t *)a2);
        break;

    case SYS_ACCEPT:
        res = (uint32_t)frontdoor_accept((int)a1);
        break;

    case SYS_SEND:
        res = (uint32_t)frontdoor_send((int)a1, (const uint8_t *)a2, (uint16_t)a3);
        break;

    case SYS_RECV:
        res = (uint32_t)frontdoor_recv((int)a1, (uint8_t *)a2, (uint16_t)a3);
        break;

    case SYS_SENDTO:
        res = (uint32_t)frontdoor_sendto((int)a1, (const sendto_args_t *)a2);
        break;

    case SYS_RECVFROM:
        res = (uint32_t)frontdoor_recvfrom((int)a1, (recvfrom_args_t *)a2);
        break;

    case SYS_SOCKCLOSE:
        res = (uint32_t)frontdoor_close((int)a1);
        break;

    default:
        res = (uint32_t)-1;
        break;
    }
    regs->eax = res;
}

void sys_print(char *user_string)
{
    if (!user_string)
        return;
    for (int i = 0; user_string[i]; i++)
        kput_char(user_string[i]);
}