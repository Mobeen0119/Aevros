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
#include "../Drivers/PIT/pit.h"
#include "pic.h"


void user_program() {
    kprint("User Program");
    volatile char *v = (volatile char*)0xB8000;
    int i = 0;
    while(1) {
        v[12] = 'U'; v[13] = 0x0C;  
        i++;
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
//   kprint("GDT OK\n");

    idt_init();
    // kprint("IDT OK\n");

    pic_remap();
    // kprint("Pic b \n");

   pmm_init(0x200000, 0x200000);   
//    kprint("PMM OK\n");

    paging_init();
//  kprint("PAGING OK\n");

    buddy_init(0x800000, 0x2000000);
//    kprint("Buddy OK\n");

    slab_init_all();
    //  kprint("SLAB OK\n");

    vfs_init();
    //  kprint("VFS OK\n");

    tty_init();
//   kprint("TTY OK\n");

    devfs_init();
//    kprint("Devfs OK\n");

    init_tasking();
//    kprint("TASKING OK\n");

    task_create_user(user_program);
    // kprint("TASKPRINT \n");

    v[20] = '!';
    v[21] = 0x0E;
    
    pit_init(100);
    // kprint("PIT OK\n");
    asm volatile("sti");
    asm volatile("int $32");
   while(1) asm volatile("hlt");
}