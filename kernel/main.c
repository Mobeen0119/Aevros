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
#include "Process/AevrosPoint/aevrospoint.h"
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
#include "Process/forktest_blob.h"

extern uint32_t kernel_end; 

#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002
#define MULTIBOOT_INFO_MEMORY      0x00000001

typedef struct multiboot_info
{
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
} multiboot_info_t;

void kernel_main(uint32_t mb_magic, uint32_t mb_info_addr)
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

    uint32_t free_start = ((uint32_t)&kernel_end + 0xFFF) & ~0xFFFu;

    uint32_t pmm_size = 0x400000;
    pmm_init(free_start, pmm_size);

    paging_init();

    uint32_t desired_end = 0x2800000; 
    uint32_t detected_end = 0;

    if (mb_magic == MULTIBOOT_BOOTLOADER_MAGIC && mb_info_addr)
    {
        multiboot_info_t *mbi = (multiboot_info_t *)mb_info_addr;
        if (mbi->flags & MULTIBOOT_INFO_MEMORY)
        {
            uint32_t total_ram = 0x100000 + mbi->mem_upper * 1024u;
            detected_end = total_ram > 0x100000 ? total_ram - 0x100000 : total_ram;
        }
    }

    uint32_t buddy_start = free_start + pmm_size;
    uint32_t buddy_end;

    if (detected_end != 0)
        buddy_end = (detected_end < desired_end) ? detected_end : desired_end;
    else
        buddy_end = buddy_start + 0x800000; 

    if (buddy_end <= buddy_start)
        buddy_end = buddy_start + 0x100000;

    buddy_init(buddy_start, buddy_end);
    slab_init_all();
    tracker_init();
     vfs_init();
    aevrospoint_init(); 
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

    {
        int fd = sys_open("/forktest.elf", READ_WRITE | CREAT);
        if (fd >= 0)
        {
            sys_write(fd, (uint8_t *)forktest_blob, forktest_blob_len);
            sys_close(fd);
        }
    }

    pit_init(100);
    asm volatile("sti");

    set_color(VGA_MAGENTA, VGA_BLACK);
    kprintf("                    Welcome to AevrosOS\n");
    reset_color();
    set_color(VGA_DARK_GREY, VGA_BLACK);
    kprintf("        ------------------------------------------------\n");
    reset_color();
    set_color(VGA_CYAN, VGA_BLACK);
    kprintf("         Not sure where to start? Try one of these:\n");
    reset_color();
    set_color(VGA_WHITE, VGA_BLACK);
    kprintf("           help");
    set_color(VGA_DARK_GREY, VGA_BLACK);
    kprintf("      - list every command\n");
    set_color(VGA_WHITE, VGA_BLACK);
    kprintf("           health");
    set_color(VGA_DARK_GREY, VGA_BLACK);
    kprintf("    - check on the system's health\n");
    set_color(VGA_WHITE, VGA_BLACK);
    kprintf("           ps");
    set_color(VGA_DARK_GREY, VGA_BLACK);
    kprintf("        - see what's running\n");
    set_color(VGA_WHITE, VGA_BLACK);
    kprintf("           identity");
    set_color(VGA_DARK_GREY, VGA_BLACK);
    kprintf("  - full system dashboard\n");
    reset_color();
    set_color(VGA_DARK_GREY, VGA_BLACK);
    kprintf("        ------------------------------------------------\n");
    reset_color();
    render();

    {
        static uint32_t boot_ctx;
        context_switch(&boot_ctx, current_task->context_esp);
    }

    while (1)
        asm volatile("hlt");
}