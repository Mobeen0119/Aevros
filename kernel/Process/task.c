#include "task.h"

struct Process *current, ready_queue;

uint32_t current_page_directory()
{
    uint32_t pd_phy;
    asm volatile("mov %%cr3,%0" : "=r"(pd_phy));
    return pd_phy;
}

void init_tasking()
{
    current = (task_t *)kmalloc(sizeof(task_t));
    current->id = 1;
    current->ebp = 0;
    current->esp = 0;
    current->page_directory = current_page_directory();
    current->next = 0;

    ready_queue = current;
}