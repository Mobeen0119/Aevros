#include "task.h"
#include "../Memory/pmm.h"
#include "../Paging/paging.h" 
#include "../Memory/kheap.c"
#include "../../Include/vfs.h"

extern uint32_t read_cr3(void);
extern uint32_t read_eip(void);

int do_fork()
{
    task_t *parent = current_task;

    if (!parent)
        return VFS_ERR;

    task_t *child = (task_t *)kmalloc(sizeof(task_t));
    if (!child)
        return VFS_ERR;

    memcpy(child, parent, sizeof(task_t));

    child->pid = next_pid++;
    child->state = TASK_READY;
    child->next = NULL;

    uint8_t *new_stack = (uint8_t *)kmalloc(4096);
   
    if (!new_stack){
        kfree(child);
        return VFS_ERR;
    }

    uint32_t stack_top = (uint32_t)new_stack + 4096;
    child->kernel_stack = stack_top;

    uint32_t stack_used = parent->kernel_stack - parent->esp;
    child->esp = stack_top - stack_used;
    child->ebp = parent->ebp + (child->esp - parent->esp);

    memcpy((void *)child->esp, (void *)parent->esp, stack_used);

    if (parent->cr3 != read_cr3())
        child->cr3 = clone_page_directory();

    for (int i = 0; i < 32; i++)
    {
        if (parent->fd_table[i] != NULL)
            parent->fd_table[i]->inode->ref_count++;
    }

    if (!ready_queue)
    {
        ready_queue = child;
        child->next = child;
    }
    else
    {
        task_t *temp = ready_queue;
        while (temp->next != ready_queue)
            temp = temp->next;

        temp->next = child;
        child->next = ready_queue;
    }

    return child->pid; 

}
