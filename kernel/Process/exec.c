#include "exec.h"

#include "task.h"
#include "../Paging/paging.h"
#include "../Memory/pmm.h"
#include "../Memory/kheap.c"

#include "../../Include/vfs.h"

#define USER_CODE 0x400000
#define USER_STACK 0x800000

int exec_user(void *binary, uint32_t size)
{
    if (!binary || !size)
        return VFS_ERR;

    uint32_t new_cr3 = clone_page_directory(read_cr3());

    asm volatile("mov %0,%%cr3" ::"a"(new_cr3));

    uint32_t code_phy = pmm_alloc();

    map_page(USER_CODE, code_phy, PAGE_PRESENT | PAGE_USER | PAGE_WRITE);

    memcpy((void *)USER_CODE, binary, size);

    uint32_t stack_phy = pmm_alloc();

    map_page(USER_STACK - 0x1000, stack_phy, PAGE_PRESENT | PAGE_USER | PAGE_WRITE);

    task_t *task = kmalloc(sizeof(task_t));

    if (!task)
        return VFS_ERR;

    memset(task, 0, sizeof(task_t));

    task->pid = next_pid++;
    task->state = TASK_READY;
    task->cr3 = new_cr3;
    task->eip = USER_CODE;

    task->esp = USER_STACK;
    task->ebp = USER_STACK;

    task->kernel_stack = (uint32_t)kmalloc(4096) + 4096;

    for (int i = 0; i < 32; i++)
    {
        task->fd_table[i] = current_task->fd_table[i];

        if (task->fd_table[i])
            task->fd_table[i]->ref_count++;
    }

    if (!ready_queue)
    {
        ready_queue = task;
        task->next = task;
    }
    else
    {
        task_t *temp = ready_queue;

        while (temp->next != ready_queue)
            temp = temp->next;
        temp->next = task;
        task->next = ready_queue;
    }

    return task->pid;
}