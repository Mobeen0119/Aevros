#include "task.h"
#include "../Memory/pmm.h"
#include "../Paging/paging.h"
#include "../../Include/vfs.h"

#include "../../Include/screen.h"
#include "../../Include/shell.h"
#include "../Dev/dev.h"
#include "../Memory/kheap.h"
#include "../CPU/TSS.h"

#include "userspace.h"
#include "process-memory/process_memory.h"
#include "../../Lib/kprintf.h"
#include "../../Lib/string.h"
#include "../../Drivers/PIT/pit.h"
#include "TaskLife/tasklife.h"

#include "../CPU/TSS.h"

task_t *current_task = 0, *ready_queue = 0;
task_t *all_tasks = 0;

int next_pid = 0;

void task_inherit_fds(task_t *child, task_t *parent)
{
    for (int i = 0; i < TASK_MAX_FDS; i++)
    {
        child->fd_table[i] = parent->fd_table[i];

        if (child->fd_table[i] &&
            child->fd_table[i]->inode)
        {
            child->fd_table[i]->inode->ref_count++;
        }
    }
}

void task_register_all(task_t *task)
{
    if (!task)
        return;

    task->all_next = all_tasks;
    all_tasks = task;
}

static inline uint32_t read_cr3()
{
    uint32_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

void init_tasking()
{
    current_task = (task_t *)kmalloc_raw(sizeof(task_t));
    if (!current_task)
        return;

    memset(current_task, 0, sizeof(task_t));

    strncpy(current_task->name, "shell", TASK_NAME_LEN);
    current_task->pid = next_pid++;
    current_task->state = TASK_RUNNING;
    current_task->cwd = vfs_root;

    memset(current_task->fd_table, 0, sizeof(current_task->fd_table));

    inode_t *tty = devfs_get("tty");
    if (tty)
    {
        file_t *stdin = kmalloc_raw(sizeof(file_t));

        stdin->inode = tty;
        stdin->flags = READ_ONLY;
        stdin->dentry = NULL;
        stdin->offset = 0;

        file_t *stdout = kmalloc_raw(sizeof(file_t));
        stdout->inode = tty;
        stdout->flags = WRITE_ONLY;
        stdout->offset = 0;
        stdout->dentry = NULL;

        file_t *stderr = kmalloc_raw(sizeof(file_t));
        stderr->offset = 0;
        stderr->inode = tty;
        stderr->flags = WRITE_ONLY;

        current_task->fd_table[0] = stdin;
        current_task->fd_table[1] = stdout;
        current_task->fd_table[2] = stderr;
    }

    uint8_t *stack_base = kmalloc_raw(4096);
    if (!stack_base)
    {
        kfree_raw(current_task);
        current_task = NULL;
        return;
    }

    uint32_t stack_top = (uint32_t)stack_base + 4096;

    current_task->context_esp = build_initial_stack(stack_base, (uint32_t)shell_start, 0x08, 0x10, stack_top);

    current_task->cr3 = read_cr3();
    current_task->regs.eip = (uint32_t)shell_start;
    current_task->kernel_stack = stack_top;
    current_task->kernel_stack_base = (uint32_t)stack_base;
    current_task->is_user = 0;
    current_task->first_run = 1;

    current_task->next = current_task;
    ready_queue = current_task;
    task_register_all(current_task);

    task_log_event(current_task, EVT_CREATED, 0);
}

task_t *task_create_kernel(void (*entry_point)())
{

    task_t *t = create_process(entry_point, 0, 0, 0);

    if (t)
    {
        strncpy(t->name, "kernel", TASK_NAME_LEN);
        task_add_ready(t);
    }
    return t;
}

task_t *task_create_user(void (*entry_point)())
{
    uint32_t page_dir = create_user_space();
    if (!page_dir)
        return NULL;

    uint32_t user_stack_top = 0xBFFF0000;

    uint32_t phys = pmm_alloc();
    uint32_t phys2 = pmm_alloc();
    if (!phys || !phys2)
    {
        if (phys)
            pmm_free(phys);
        if (phys2)
            pmm_free(phys2);
        destroy_user_space(page_dir);
        return NULL;
    }

    map_page_in_directory(page_dir, user_stack_top, phys, PAGE_PRESENT | PAGE_USER | PAGE_WRITE);
    map_page_in_directory(page_dir, user_stack_top - 4096, phys2, PAGE_PRESENT | PAGE_USER | PAGE_WRITE);

    task_t *task = create_process(entry_point, 0, page_dir, user_stack_top + 4096 - 16);

    if (!task)
    {
        destroy_user_space(page_dir);
        return NULL;
    }

    task_add_ready(task);
    return task;
}

uint32_t build_initial_stack(uint8_t *stack_base, uint32_t entry_point, uint32_t cs, uint32_t ss, uint32_t user_esp)
{
    uint32_t stack_top = (uint32_t)stack_base + 4096;
    uint32_t *sp = (uint32_t *)stack_top;

    *(--sp) = ss;
    *(--sp) = user_esp;
    *(--sp) = 0x202;
    *(--sp) = cs;
    *(--sp) = entry_point;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = ss;

    *(--sp) = (uint32_t)trap_return;
    *(--sp) = 0x202;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;

    return (uint32_t)sp;
}

task_t *create_process(void (*entry_point)(), uint32_t flags, uint32_t page_dir, uint32_t user_stack_top)

{
    (void)flags;

    task_t *new_task = (task_t *)kmalloc_raw(sizeof(task_t));
    if (!new_task)
        return NULL;
    memset(new_task, 0, sizeof(task_t));

    new_task->pid = next_pid++;
    new_task->state = TASK_READY;
    new_task->parent = current_task;
    new_task->first_run = 1;

    uint8_t *stack_base = kmalloc_raw(4096);
    if (!stack_base)
    {
        kfree_raw(new_task);
        return NULL;
    }

    uint32_t stack_top = (uint32_t)stack_base + 4096;

    uint32_t cs, ss;
    if (page_dir)
    {
        cs = 0x1B;
        ss = 0x23;
    }
    else
    {
        cs = 0x08;
        ss = 0x10;
    }

    new_task->context_esp = build_initial_stack(stack_base, (uint32_t)entry_point, cs, ss,
                                                 page_dir ? user_stack_top : stack_top);

    new_task->cr3 = page_dir ? page_dir : read_cr3();

    new_task->regs.eip = (uint32_t)entry_point;
    new_task->kernel_stack = stack_top;

    new_task->kernel_stack_base = (uint32_t)stack_base;
    new_task->is_user = (page_dir != 0) ? 1 : 0;
    task_inherit_fds(new_task, current_task);

    task_register_all(new_task);

    return new_task;
}

void schedule(void)

{
    task_t *prev = current_task;
    task_t *next = pick_next_task();

    if (!next || next == prev || !prev)
        return;

    if (prev->state == TASK_RUNNING)
        prev->state = TASK_READY;

    next->state = TASK_RUNNING;
    current_task = next;

    if (next->cr3)
        asm volatile("mov %0, %%cr3" ::"r"(next->cr3) : "memory");

    context_switch(&prev->context_esp, next->context_esp);

}

void switch_to_task_state(task_t *target, task_state_t leave_prev_as)
{
    task_t *prev = current_task;
    if (!target || !prev || target == prev)
        return;

    prev->state = leave_prev_as;
    target->state = TASK_RUNNING;
    current_task = target;

    if (target->cr3)
        asm volatile("mov %0, %%cr3" ::"r"(target->cr3) : "memory");

    context_switch(&prev->context_esp, target->context_esp);
}

__attribute__((noinline)) void sys_exit(int status)
{
    task_t *dead = current_task;
    if (!dead)
        return;

    task_log_event(dead, EVT_EXITED, (uint32_t)status);

    dead->destroy_time = get_ticks();
    dead->exit_code = status;
    dead->state = TASK_ZOMBIE;

    if (dead->parent)
        task_log_event(dead->parent, EVT_CHILD_DIED, dead->pid);

    for (int i = 0; i < 32; i++)
        if (dead->fd_table[i])
            sys_close(i);

    if (dead->next == dead)
    {
        kprint("System Halted : All Processes exited\n");
        while (1)
            asm volatile("hlt");
    }

    task_t *temp = ready_queue;
    while (temp->next != dead)
        temp = temp->next;

    temp->next = dead->next;
    if (ready_queue == dead)
        ready_queue = dead->next;

    if (dead->is_user && dead->cr3)
        destroy_user_space(dead->cr3);

    if (dead->parent && dead->parent->state == TASK_BLOCKED)
        dead->parent->state = TASK_READY;

    task_t *next = pick_next_task();

    if (!next || next == dead)
    {
        kprint("System Halted : No runnable tasks\n");
        while (1)
            asm volatile("hlt");
    }

    next->state = TASK_RUNNING;
    current_task = next;

    if (next->cr3)
        asm volatile("mov %0, %%cr3" ::"r"(next->cr3) : "memory");

    context_switch(&dead->context_esp, next->context_esp);

    __builtin_unreachable();
}

int sys_waitpid(int target_pid, int *status)
{

    task_t *parent = current_task;
    if (!parent)
        return VFS_ERR;

    while (1)
    {
        int has_children = 0;

        task_t *curr = all_tasks;

        while (curr)

        {
            if (curr->parent == parent && (target_pid == -1 || curr->pid == target_pid))
            {
                has_children = 1;

                if (curr->state == TASK_ZOMBIE)
                {
                    if (status != NULL)
                        *status = curr->exit_code;

                    task_t *linker = all_tasks;
                    if (linker == curr)
                    {
                        all_tasks = curr->all_next;
                    }
                    else
                    {
                        while (linker->all_next != curr)
                            linker = linker->all_next;
                        linker->all_next = curr->all_next;
                    }

                    int dead_pid = curr->pid;

                    if (curr->kernel_stack_base)
                        kfree_raw((void *)(curr->kernel_stack_base));
                    kfree_raw(curr);

                    return dead_pid;
                }
            }

            curr = curr->all_next;
        }

        if (!has_children)
            return VFS_ERR;

        parent->state = TASK_BLOCKED;
        schedule();
    }
}

void task_add_ready(task_t *task)
{
    if (!task)
        return;

    task->state = TASK_READY;
    if (ready_queue == NULL)
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
}

void task_remove_ready(task_t *task)
{
    if (!task || !ready_queue)
        return;

    task_log_event(task, EVT_BLOCKED, 0);
    task->state = TASK_BLOCKED;

    if (ready_queue == task && ready_queue->next == ready_queue)
    {
        ready_queue = NULL;
        return;
    }

    task_t *temp = ready_queue;
    do
    {
        if (temp->next == task)
            break;
        temp = temp->next;
    } while (temp->next != ready_queue);

    if (temp->next != task)
        return;
    temp->next = task->next;

    if (ready_queue == task)
        ready_queue = task->next;
}

task_t *pick_next_task(void)
{
    if (!ready_queue)
        return NULL;

    task_t *start = (current_task && current_task->next) ? current_task->next : ready_queue;
    task_t *temp = start;
    task_t *fallback = NULL;

    for (uint32_t guard = 0; guard < 0xFFFF; guard++)
    {
        if (temp->state == TASK_READY && temp != current_task)
            return temp;

        if (!fallback && temp->state == TASK_READY)
            fallback = temp;

        temp = temp->next;
        if (!temp)
            break;
        if (temp == start)
            break;
    }

    if (current_task && current_task->state == TASK_READY)
        return current_task;

    return fallback;
}

uint32_t get_ticks(void)
{
    return timer_clicks;
}

void task_wake(task_t *task)
{
    if (!task)
        return;

    task_log_event(task, EVT_WOKE, 0);
    task->state = TASK_READY;
}