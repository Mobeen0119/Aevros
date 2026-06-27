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

static inline void user_exit(int code)
{
    asm volatile("mov %0, %%eax\n"
                 "mov %1, %%ebx\n"
                 "int $0x80"
                 :
                 : "i"(SYS_EXIT), "r"(code)
                 : "eax", "ebx");
}

void user_program()
{
    outb(0xE9, 'U');
    syscall(SYS_WRITE, 1, (int)"seven\n", 6);
    outb(0xE9, 'W');
    syscall(SYS_EXIT, 0, 0, 0);
    outb(0xE9, 'X');
    while (1) { }
}


void fork_test_program(void)
{
    int result = syscall(SYS_FORK, 0, 0, 0);

    if (result == 0)
    {
        syscall(SYS_WRITE, 1, (int)"child running\n", 14);
        syscall(SYS_EXIT, 42, 0, 0);
    }
    else if (result > 0)
    {
        syscall(SYS_WRITE, 1, (int)"parent: forked child\n", 22);
        syscall(SYS_EXIT, 0, 0, 0);
    }
    else
    {
        syscall(SYS_WRITE, 1, (int)"fork failed\n", 12);
        syscall(SYS_EXIT, 1, 0, 0);
    }
    while(1) { }
}

void kernel_main()
{
    outb(0xE9,'@');

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
    kprintf("OFFSETS: cr3=%u esp=%u ebp=%u eip=%u kstack=%u first_run=%u\n",
   
        (unsigned)offsetof(task_t, cr3),
    (unsigned)offsetof(task_t, regs.esp),
    (unsigned)offsetof(task_t, regs.ebp),
    (unsigned)offsetof(task_t, regs.eip),
    (unsigned)offsetof(task_t, kernel_stack),
    (unsigned)offsetof(task_t, first_run));
        
    slab_init_all();

    tracker_init();

    vfs_init();
    ramfs_init();

    tty_init();
    devfs_init();
    init_tasking();

    kprintf("BEFORE TASK CREATE\n");

     task_create_user(user_program);
    task_create_user(fork_test_program);
    kprintf("AFTER TASK CREATE\n");
    pit_init(100);
    asm volatile("sti");
    kprintf("STI DONE\n");

    kprintf("\nForgeOS ready. Try: ps, memstory, fdleak, outlook\n");

    {
        static uint32_t boot_context_discard;
        task_t *shell_task = current_task;
        context_switch(&boot_context_discard, shell_task->context_esp);
    }

    while (1)
        asm volatile("hlt");
}
