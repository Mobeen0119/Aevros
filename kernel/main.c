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

void kernel_main(unsigned int magic, unsigned int mboot_ptr) {
    
    
    terminal_initialize(); 
    kprint("Welcome to ForgeOS!\n");
    kprint("-------------------\n");

  
    gdt_init();          
    idt_init();          
    tss_init();          
    kprint("[OK] CPU Core Frameworks Online.\n");

   
    pmm_init(mboot_ptr,sizeof(mboot_ptr)); 
    paging_init();       
    kprint("[OK] Memory Management Sandboxing Active.\n");

    vfs_root = vfs_init(); 
 
    // ramfs_mkdir("/bin");
    // ramfs_mkdir("/dev");
    
    devfs_init(); 
    kprint("[OK] Filesystems mounted and hardware devices mapped.\n");

    init_tasking();      
    init_syscalls();     
    kprint("[OK] Scheduler running. Multi-process loops ready.\n");

    kprint("Booting into user environment...\n");
    
    sys_execve("/bin/test"); 

    asm volatile("sti");

    while(1) {
        asm volatile("hlt");
    }
}