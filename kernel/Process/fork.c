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

    memset(child, 0, sizeof(task_t));

    uint32_t eip = read_eip();

    child->pid = next_pid++;
    child->state = TASK_READY;
    child->next = NULL;
    child->cwd = parent->cwd;

    child->parent = parent;
    child->regs.eip = eip;
    child->regs.eax = 0;
    child->exit_code = 0;

    uint8_t *new_stack = (uint8_t *)kmalloc(4096);

    if (!new_stack)
    {
        kfree(child);
        return VFS_ERR;
    }

    if (!child->cr3)
    {
        kfree(new_stack);
        kfree(child);
        return VFS_ERR;
    }

    uint32_t stack_top = (uint32_t)new_stack + 4096;
    child->kernel_stack = stack_top;

    uint32_t stack_used = parent->kernel_stack - parent->regs.esp;
    child->regs.esp = stack_top - stack_used;
    child->regs.ebp = parent->regs.ebp + (child->regs.esp - parent->regs.esp);

    memcpy((void *)child->regs.esp, (void *)parent->regs.esp, stack_used);

    child->cr3 = clone_page_directory(parent->cr3);

    for (int i = 0; i < 32; i++)
    {
        child->fd_table[i] = parent->fd_table[i];

        if (child->fd_table[i])
            child->fd_table[i]->inode->ref_count++;
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
