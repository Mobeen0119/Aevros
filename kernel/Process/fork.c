#include "task.h"
#include "../Memory/pmm.h"
#include "../Paging/paging.h"
#include "../Memory/kheap.h"
#include "../../Include/vfs.h"
#include "TaskLife/tasklife.h"
#include "process-memory/process_memory.h"
#include "../../Lib/string.h"
#include "../../Lib/kprintf.h"

extern uint32_t read_cr3(void);
extern uint32_t read_eip(void);

int do_fork(register_t *state_at_interuppt)
{
    if (!current_task || !state_at_interuppt)
    {
        return -VFS_ENOMEM;
    }

    task_t *parent = current_task;

    task_t *child = (task_t *)kmalloc_raw(sizeof(task_t));
    if (!child)
    {
        return VFS_ERR;
    }
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
    child->state = TASK_READY;
    child->next = NULL;
    child->first_run = 1;
    child->is_user = 1;

    uint32_t stack_top = (uint32_t)new_stack + 4096;
    child->kernel_stack = stack_top;
    child->kernel_stack_base = (uint32_t)new_stack;

    uint32_t *sp = (uint32_t *)stack_top;
    *(--sp) = state_at_interuppt->ss;
    *(--sp) = state_at_interuppt->useresp;
    *(--sp) = state_at_interuppt->eflags;
    *(--sp) = state_at_interuppt->cs;
    *(--sp) = state_at_interuppt->eip;
    *(--sp) = state_at_interuppt->err_code;
    *(--sp) = state_at_interuppt->int_no;
    *(--sp) = 0; 
    *(--sp) = state_at_interuppt->ecx;
    *(--sp) = state_at_interuppt->edx;
    *(--sp) = state_at_interuppt->ebx;
    *(--sp) = 0; 
    *(--sp) = state_at_interuppt->ebp;
    *(--sp) = state_at_interuppt->esi;
    *(--sp) = state_at_interuppt->edi;
    *(--sp) = state_at_interuppt->ds;

    *(--sp) = (uint32_t)trap_return; 
    *(--sp) = 0x202; 
    *(--sp) = 0; 
    *(--sp) = 0; 
    *(--sp) = 0; 
    *(--sp) = 0; 

    child->context_esp = (uint32_t)sp;
    child->regs.eip = state_at_interuppt->eip;

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
    strncpy(child->name, parent->name, TASK_NAME_LEN - 1);
    child->name[TASK_NAME_LEN - 1] = '\0';
    child->start_time = get_ticks();

    task_log_event(parent, EVT_FORKED, child->pid);
    task_log_event(child, EVT_CREATED, parent->pid);

    int pid = child->pid;
    task_register_all(child);

    task_add_ready(child);
    return pid;
    
}
