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
#include "Dev/dev.h"
#include "../Drivers/tty.h"

void user_program()
{
    while (1)
    {
        kprint("HELLO USER\n");
    }
}

void kernel_main() {
    volatile char *v = (volatile char *)0xB8000;
    for (int i = 0; i < 80*25*2; i+=2) { v[i]=' '; v[i+1]=0x07; }

    asm volatile("cli");
    gdt_init();    v[0]='G'; v[1]=0x0A;
    idt_init();    v[2]='I'; v[3]=0x0A;
    pmm_init(0x200000, 0x200000);
                   v[4]='P'; v[5]=0x0A;
    paging_init(); v[6]='A'; v[7]=0x0A;
    buddy_init(0x800000, 0x2000000);
                   v[8]='B'; v[9]=0x0A;
    slab_init_all();
                   v[10]='S'; v[11]=0x0A;
    vfs_init();    v[12]='V'; v[13]=0x0A;
    tty_init();    v[14]='T'; v[15]=0x0A;  
    devfs_init();  v[16]='D'; v[17]=0x0A;
    init_tasking();
                   v[18]='K'; v[19]=0x0A;
    asm volatile("sti");
                   v[20]='!'; v[21]=0x0E;

    while(1) asm volatile("hlt");
}