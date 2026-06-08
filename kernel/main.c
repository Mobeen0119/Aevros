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
#include "Memory/buddy.h"
#include "Memory/slab.h"
#include "../Include/vfs.h"


void user_program()
{
    while (1)
    {
        kprint("HELLO USER\n");
    }
}

void kernel_main()
{
    volatile char *v = (volatile char *)0xB8000;
    for (int i = 0; i < 80 * 25 * 2; i += 2)
    {
        v[i] = ' ';
        v[i + 1] = 0x07;
    }

    asm volatile("cli");
    gdt_init();
    v[0] = 'K';
    v[1] = 0x0A;
    idt_init();
    v[2] = 'E';
    v[3] = 0x0A;

    pmm_init(0x200000, 0x200000);
    v[4] = 'R';
    v[5] = 0x0A;
    paging_init();
    v[6] = 'N';
    v[7] = 0x0A;

    buddy_init(0x400000, 0x400000);
    v[6] = 'E';
    v[7] = 0x0A;
    slab_init_all();
    v[6] = 'L';
    v[7] = 0x0A;
    init_tasking();
    v[8] = 'S';
    v[9] = 0x0A;

     vfs_init();
          v[10]='H'; v[11]=0x0A;
    devfs_init();
         v[10]='I'; v[11]=0x0A;
    ramfs_init();
         v[10]='T'; v[11]=0x0A;
    terminal_init();
         v[10]='N'; v[11]=0x0A;

     task_create_user(user_program);
                  v[10]='O'; v[11]=0x0A;

    asm volatile("sti");
    v[12] = '!';
    v[13] = 0x0E;

    while (1)
        asm volatile("hlt");
}