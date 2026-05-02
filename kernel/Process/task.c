#include "task.h"
#include "pmm.c"
#include "pmm.h"
#include "Paging/paging.c"
#define Temp_p_vir_addr 0xFFC00000

task_t *current, *ready_queue;
int next_pid = 0;

uint32_t current_page_directory()
{
    uint32_t pd_phy;
    asm volatile("mov %%cr3,%0" : "=r"(pd_phy));
    return pd_phy;
}

void init_tasking()
{
    current = (task_t *)kmalloc(sizeof(task_t));
    if (!current)
        return;

    current->id = next_pid++;

    asm volatile("mov %%esp, %0" : "=r"(current->esp));
    asm volatile("mov %%ebp, %0" : "=r"(current->ebp));

    current->page_directory = current_page_directory();
    current->next = 0;

    ready_queue = current;
}

task_t *create_process(void (*entry_point)(), uint32_t flags, uint32_t page_dir)
{

    task_t *new_task = (task_t *)kmalloc(sizeof(task_t));
    if (!new_task)
        return 0;
    new_task->next = 0;

    uint32_t stack_base = kmalloc(4096);
    if (!stack_base)
        return 0;

    uint32_t stack_top = (stack_base) + 4096;

    uint32_t *sp = (uint32_t *)stack_top;
    *(--sp) = 0;

    new_task->id = next_pid++;
    if (page_dir)
        new_task->page_directory = page_dir;
    else
        new_task->page_directory = (uint32_t)clone_kernel_directory();

    new_task->ebp = new_task->esp = (uint32_t)sp;
    new_task->eip = (uint32_t)entry_point;

    if (!ready_queue)
        ready_queue = new_task;
    else
    {
        task_t *temp = ready_queue;
        while (temp->next)
            temp = temp->next;
        temp->next = new_task;
    }
    return new_task;
}

page_directory_t *clone_kernel_directory()
{
    uint32_t pd_phy = pmm_alloc();

    map_page(Temp_p_vir_addr, pd_phy, 0x03);

    page_directory_t *page_virtual = (page_directory_t *)Temp_p_vir_addr;

    memset(page_virtual, 0, 4096);

    for (int i = 768; i < 1023; i++)
    {
        page_virtual->entries[i] = kernel_directory->entries[i];
    }

    page_virtual->entries[1023] = pd_phy | 0x03;
    unmap(Temp_p_vir_addr);

    return (page_directory_t *)pd_phy;
}