// kernel/main.c
#include "../Include/screen.h"
#include "../Include/vfs.h"
#include "../Include/ramfs.h"
#include "../Include/terminal.h"
#include "CPU/GDT.h"
#include "CPU/idt.h"
#include "CPU/TSS.h"
#include "Memory/pmm.h"
#include "Paging/paging.h"
#include "Process/task.h"
#include "Process/exec.h"

void user_program()
{
    while (1)
    {
        kprint("HELLO USER\n");
    }
}

void kernel_main()
{

    volatile char *vga = (volatile char *)0xB8000;
    vga[0] = 'X';
    vga[1] = 0x0F;

    init_pmm();
    init_paging();
    init_tasking();
    init_interrupts();

    task_create_user(user_program);

    asm volatile("sti"); // enable interrupts
    while (1)
        ;
}