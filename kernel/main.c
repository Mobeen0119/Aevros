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
#include "Process/ForgePoint/forgepoint.h"
#include "Process/FDLeak/fdleak.h"
#include "Process/OutLook/outlook.h"
#include "Process/Quarantine/Quarantine.h"
#include "Process/Blast/blast.h"
#include "Process/WhyAlive/whyalive.h"
#include "Memory/MemFreeze/memfreeze.h"
#include "Memory/buddy.h"
#include "Memory/slab.h"
#include "../Include/shell.h"
#include "Dev/dev.h"
#include "../Drivers/tty.h"
#include "../Drivers/PIT/pit.h"
#include "../Lib/kprintf.h"
#include "pic.h"
#include "io.h"
#include "Syscall/syscall.h"
#include "Process/exectest_blob.h"

extern uint32_t kernel_end; 

void kernel_main()
{
    volatile char *v = (volatile char *)0xB8000;
    for (int i = 10; i < 80 * 25 * 2; i += 2)
    {
        v[i] = ' ';
        v[i + 1] = 0x07;
    }

    asm volatile("cli");

    gdt_init();
    idt_init();

    pic_remap();

    uint32_t free_start = (uint32_t)&kernel_end;
    pmm_init(free_start, 0x2000000 - free_start);   
    paging_init();
    buddy_init(0x800000, 0x2000000); 
    slab_init_all();
    tracker_init();
    vfs_init();
    ramfs_init();
    tty_init();
    devfs_init();
    init_tasking();

    {
        int fd = sys_open("/exectest.elf", READ_WRITE | CREAT);
        if (fd >= 0)
        {
            sys_write(fd, (uint8_t *)exectest_blob, exectest_blob_len);
            sys_close(fd);
        }
    }

    pit_init(100);
    asm volatile("sti");

    kprintf("Welcome to ForgeOS\n");
    set_color(VGA_CYAN, VGA_DARK_GREY);
    kprintf("\t\t\t\tType a command to get started.\n\n");
    set_color(VGA_GREEN, VGA_BLACK);
    kprintf("\nNyros > ");
    reset_color();

    {
        static uint32_t boot_ctx;
        context_switch(&boot_ctx, current_task->context_esp);
    }

    while (1)
        asm volatile("hlt");
}
