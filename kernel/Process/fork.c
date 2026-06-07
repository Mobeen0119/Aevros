#include "task.h"
#include "../Memory/pmm.h"
#include "../Paging/paging.h"
#include "../Memory/kheap.h"
#include "../../Include/vfs.h"
#include "task.h"
#include "process-memory/process_memory.h"

extern uint32_t read_cr3(void);
extern uint32_t read_eip(void);

int do_fork(register_t *state_at_interuppt)
{
    if(!current_task || !state_at_interuppt) return VFS_ERR;

    task_t *parent = current_task;

    if (!parent)
        return VFS_ERR;

    task_t *child = (task_t *)kmalloc(sizeof(task_t));

    if (!child)
        kfree(child);
        return VFS_ERR;

    memset(child, 0, sizeof(task_t));

    child->cr3 = clone_page_directory(parent->cr3);

    if (!child->cr3)
    {
        kfree(child);
        return VFS_ERR;
    }
    
    uint8_t *new_stack = (uint8_t *)kmalloc(4096);

    if (!new_stack)
    {
        destroy_user_space(child->cr3);
        kfree(child);
        return VFS_ERR;
    }

    child->pid = next_pid++;
    child->state = TASK_READY;
    child->next = NULL;
    child->cwd = parent->cwd;

    child->parent = parent;
    child->exit_code = 0;

    child->regs = *state_at_interuppt;

    child->regs.eax = 0;



    uint32_t stack_top = (uint32_t)new_stack + 4096;
    child->kernel_stack = stack_top;

    if (state_at_interuppt->esp > parent->kernel_stack)
    {
        destroy_user_space(child->cr3);
        kfree(child);
        kfree(new_stack);

        return VFS_ERR;
    }

    uint32_t stack_used = parent->kernel_stack - state_at_interuppt->esp;

    if(stack_used>4096){
        destroy_user_space(child->cr3);
        kfree(stack_used);
        kfree(child);
        
        return VFS_ERR;
    }
    child->regs.esp = stack_top - stack_used;
    child->regs.ebp = state_at_interuppt->ebp + (child->regs.esp - state_at_interuppt->esp);

    memcpy((void *)child->regs.esp, (void *)state_at_interuppt->esp, stack_used);

    for (int i = 0; i < 32; i++)
    {
        child->fd_table[i] = parent->fd_table[i];

        if (child->fd_table[i])
            child->fd_table[i]->inode->ref_count++;
    }

    task_add_ready(child);

    return child->pid;
}
