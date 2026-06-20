#include "../Include/screen.h"
#include "../Include/vfs.h"
#include "../Include/ramfs.h"
#include "../Include/terminal.h"
#include "CPU/GDT.h"
#include "CPU/idt.h"
#include "CPU/TSS.h"
#include "Memory/pmm.h"
#include "Memory/kheap.h"
#include "Memory/KallocTracker/kalloc_tracker.h"
#include "Paging/paging.h"
#include "Process/task.h"
#include "Process/exec.h"
#include "Process/TaskLife/tasklife.h"
#include "Process/Forgepoint/forgepoint.h"
#include "Process/FdLeak/fdleak.h"
#include "Process/Outlook/outlook.h"
#include "Process/Quarantine/quarantine.h"
#include "Process/Blast/blast.h"
#include "Process/Whyalive/whyalive.h"
#include "Memory/MemFreeze/memfreeze.h"
#include "Memory/buddy.h"
#include "Memory/slab.h"
#include "../Include/shell.h"
#include "Dev/dev.h"
#include "../Drivers/tty.h"
#include "../Drivers/PIT/pit.h"
#include "pic.h"

void test_leaky_task()
{
    int fd = sys_open("/leak_test.txt", READ_WRITE | CREAT);
    if (fd >= 0)
    {
        sys_write(fd, (uint8_t *)"leaked data", 11);
       
    }

    while (1)
        asm volatile("hlt");   
}

void test_forking_task()
{
    int pid = do_fork(NULL);  
    (void)pid;

    while (1)
        asm volatile("hlt");
}

void user_program()
{
    kprint("User Program");
    volatile char *v = (volatile char *)0xB8000;
    while (1)
    {
        v[12] = 'U';
        v[13] = 0x0C;
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
    idt_init();
    pic_remap();
    pmm_init(0x200000, 0x200000);
    paging_init();
    buddy_init(0x800000, 0x2000000);
    slab_init_all();

    tracker_init();     

    vfs_init();
    ramfs_init();        

    tty_init();
    devfs_init();
    init_tasking();

    task_create_user(user_program);

    pit_init(100);
    asm volatile("sti");

    kprint("\nForgeOS ready. Try: ps, memstory, fdleak, outlook\n");
    shell_start();

    while (1)
        asm volatile("hlt");
}