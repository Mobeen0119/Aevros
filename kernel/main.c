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

void kernel_main(){
    kprint("kch");
asm volatile("cli");
  kprint("kch");

    gdt_init();
      kprint("kch");
    idt_init();
      kprint("kch");

    pmm_init(0x100000, 0x4000000);
      kprint("kch");
    paging_init();

    init_tasking();
    task_create_user(user_program);

    asm volatile("sti"); 

    while (1) {
        asm volatile("hlt");
    }
}