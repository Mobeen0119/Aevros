#include "task.h"
#include "../Memory/pmm.h"
#include "../Paging/paging.h"
#include "../Memory/kheap.h"
#include "../../Include/vfs.h"
#include "task.h"
#include "TaskLife/tasklife.h"
#include "process-memory/process_memory.h"

extern uint32_t read_cr3(void);
extern uint32_t read_eip(void);

int do_fork(register_t *state_at_interuppt)
{
    if (!current_task || !state_at_interuppt)
        return -VFS_ENOMEM;

    task_t *parent = current_task;

    if (!parent)
        return VFS_ERR;

    task_t *child = (task_t *)kmalloc_raw(sizeof(task_t));

    if (!child)
        return VFS_ERR;

    memset(child, 0, sizeof(task_t));

    child->cr3 = clone_page_directory(parent->cr3);

    if (!child->cr3)
    {
        kfree_raw(child);
        return VFS_ERR;
    }

    uint8_t *new_stack = (uint8_t *)kmalloc_raw(4096);

    if (!new_stack)
    {
        destroy_user_space(child->cr3);
        kfree_raw(child);
        return -VFS_ENOMEM;
    }

    child->pid = next_pid++;
    child->parent = parent;
    child->exit_code = 0;

    child->state = TASK_SUSPENDED;
    child->next = NULL;

    child->regs = *state_at_interuppt;

    child->regs.eax = 0;

    uint32_t stack_top = (uint32_t)new_stack + 4096;
    child->kernel_stack = stack_top;
    child->kernel_stack_base = (uint32_t)new_stack;

    if (state_at_interuppt->esp > parent->kernel_stack)
    {
        destroy_user_space(child->cr3);
        kfree_raw(child);
        kfree_raw(new_stack);

        return -VFS_ENOMEM;
    }

    uint32_t stack_used = parent->kernel_stack - state_at_interuppt->esp;

    if (stack_used > 4096)
    {
        destroy_user_space(child->cr3);
        kfree_raw(new_stack);
        kfree_raw(child);

        return -VFS_ENOMEM;
    }
    child->regs.esp = stack_top - stack_used;

    memcpy((void *)child->regs.esp, (void *)state_at_interuppt->esp, stack_used);
    uint32_t stack_delta = child->regs.esp - state_at_interuppt->esp;

    if (state_at_interuppt->ebp >= state_at_interuppt->esp &&
        state_at_interuppt->ebp < parent->kernel_stack)
    {
        child->regs.ebp = state_at_interuppt->ebp + stack_delta;
    }
    else
    {
        child->regs.ebp = state_at_interuppt->ebp;
    }

    child->cwd = parent->cwd;
    if (child->cwd)
        child->cwd->ref_count++;

    for (int i = 0; i < TASK_MAX_FDS; i++)
    {
        if (!parent->fd_table[i])
            continue;

        if (parent->fd_table[i]->flags & O_CLOEXEC)
        {
            child->fd_table[i] = NULL;
            continue;
        }
        child->fd_table[i] = parent->fd_table[i];
        child->fd_table[i]->inode->ref_count++;
    }

    child->signal_mask = parent->signal_mask;

    child->pending_signals = 0;

    memcpy(child->signal_handlers, parent->signal_handlers, sizeof(parent->signal_handlers));

    memcpy(child->rlimits, parent->rlimits, sizeof(parent->rlimits));

    child->user_time = 0;
    child->kernel_time = 0;

    child->start_time = get_ticks();

    task_log_event(parent, EVT_FORKED, child->pid);
    task_log_event(child, EVT_CREATED, parent->pid);

    int pid = child->pid;
    task_add_ready(child);
    return pid;
}
