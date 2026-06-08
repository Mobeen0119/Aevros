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
    gdt_init();  // MUST be first: Sets up code/data segments
    idt_init();  // MUST be second: Prevents the "!" crash on exceptions

    // 2. Memory: Set up physical and virtual memory
    pmm_init(0x100000, 0x4000000);
    paging_init(); // Ensure paging is active before tasks exist

    // 3. System: Multitasking and User space
    init_tasking();
    task_create_user(user_program);

    // 4. Finally: Enable CPU interrupts
    asm volatile("sti"); 

    while (1);
}