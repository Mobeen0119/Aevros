#include "task.h"
#include "../Memory/pmm.h"
#include "../Paging/paging.h"
#include "../../Include/vfs.h"
#include "../io.h"
#include "../../Include/screen.h"
#include "../Dev/dev.h"
#include "../Memory/kheap.h"
#include "../CPU/tss.h"
#include "userspace.h"
#include "process-memory/process_memory.h"
#include "../../Lib/kprintf.h"

task_t *current_task = 0, *ready_queue = 0;

int next_pid = 0;

extern void switch_current_task(task_t *prev, task_t *next);
extern uint32_t read_eip();

static inline uint32_t read_cr3()
{
    uint32_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

void init_tasking()
{
    current_task = (task_t *)kmalloc(sizeof(task_t));
    if (!current_task)
        return;

    memset(current_task, 0, sizeof(task_t));
    current_task->pid = next_pid++;
    current_task->state = TASK_RUNNING;
    current_task->cwd = vfs_root;

    memset(current_task->fd_table, 0, sizeof(current_task->fd_table));

    inode_t *tty = devfs_get("tty");
    if (tty)
    {
        file_t *stdin = kmalloc(sizeof(file_t));

        stdin->inode = tty;
        stdin->flags = READ_ONLY;
        stdin->dentry = NULL;
        stdin->offset = 0;

        file_t *stdout = kmalloc(sizeof(file_t));
        stdout->inode = tty;
        stdout->flags = WRITE_ONLY;
        stdout->offset = 0;
        stdout->dentry = NULL;

        file_t *stderr = kmalloc(sizeof(file_t));
        stderr->offset = 0;
        stderr->inode = tty;
        stderr->flags = WRITE_ONLY;

        current_task->fd_table[0] = stdin;
        current_task->fd_table[1] = stdout;
        current_task->fd_table[2] = stderr;
    }
    asm volatile("mov %%esp, %0" : "=r"(current_task->regs.esp));
    asm volatile("mov %%ebp, %0" : "=r"(current_task->regs.ebp));

    current_task->cr3 = read_cr3();
    current_task->regs.eip = read_eip();
    current_task->kernel_stack = current_task->regs.esp;

    current_task->next = current_task;
    ready_queue = current_task;
}

task_t *task_create_kernel(void (*entry_point)())
{
    kprint("KERNEL CALL\n");
    return create_process(entry_point, 0, 0);
}

task_t *task_create_user(void (*entry_point)())
{
    uint32_t page_dir = create_user_space();
    if (!page_dir)
        return VFS_OK;

    task_t *task = create_process(entry_point, 0, page_dir);

    if (!task)
    {
        destroy_user_space(page_dir);
        return VFS_OK;
    }

    task->state = TASK_READY;

    kprint("TASKINGGGGGGGGG\n");
    return task;
}

task_t *create_process(void (*entry_point)(), uint32_t flags, uint32_t page_dir)
{

    task_t *new_task = (task_t *)kmalloc(sizeof(task_t));
    if (!new_task)
        return 0;

    memset(new_task, 0, sizeof(task_t));
    new_task->pid = next_pid++;
    new_task->state = TASK_READY;
    new_task->parent = current_task;

    uint8_t *stack_base = kmalloc(4096);
    if (!stack_base)
        return 0;

    uint32_t stack_top = (uint32_t)stack_base + 4096;

    uint32_t *sp = (uint32_t *)stack_top;
   
    *(--sp) = 0;                     // saved ebp
    *(--sp) = 0;                     // saved ebx
    *(--sp) = 0;                     // saved esi
    *(--sp) = 0;

    *(--sp) = (uint32_t)entry_point;
    if (page_dir)
        new_task->cr3 = page_dir;
    else
        new_task->cr3 = read_cr3();

    new_task->regs.esp = (uint32_t)sp;
    new_task->regs.ebp = stack_top;
    new_task->regs.eip = (uint32_t)entry_point;
    new_task->kernel_stack = stack_top;

    if (!ready_queue)
    {
        ready_queue = new_task;
        new_task->next = new_task;
    }
    else
    {
        task_t *temp = ready_queue;
        while (temp->next != ready_queue)
            temp = temp->next;
        temp->next = new_task;
        new_task->next = ready_queue;
    }
    return new_task;
}

void schedule()
{
    kprint("SCHEDULE RUN\n");
    if (!current_task)
    {
        kprint("ha hi ni");
        return;
    }
    if (current_task->state == TASK_RUNNING)
        current_task->state = TASK_READY;

    task_t *prev = current_task;
    task_t *next = pick_next_task();

    kprint("SWITCHING\n");

    if (!next || next == prev)
    {
        prev->state = TASK_RUNNING;
        return;
    }

    current_task = next;
    current_task->state = TASK_RUNNING;
    asm volatile("mov %0,%%cr3" ::"r"(next->cr3));

    tss.esp0 = next->kernel_stack;
    tss.ss0 = 0x10;

    kprint("Endddd\n");

    kprintf("next=%x\n", next);
kprintf("eip=%x\n", next->regs.eip);
kprintf("esp=%x\n", next->regs.esp);
kprintf("*esp=%x\n", *(uint32_t*)next->regs.esp);

    switch_current_task(prev, next);
}

void sys_exit(int status)
{
    task_t *dead = current_task;
    if (!dead)
        return;

    dead->exit_code = status;
    dead->state = TASK_ZOMBIE;

    for (int i = 0; i < 32; i++)
    {
        if (dead->fd_table[i])
        {
            sys_close(i);
        }
    }

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

    if (dead->parent && dead->parent->state == TASK_BLOCKED)
        dead->parent->state = TASK_READY;

    schedule();

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
        task_t *curr = ready_queue;

        if (!curr)
            return VFS_ERR;

        do
        {
            if (curr->parent == parent && (target_pid == -1 || curr->pid == target_pid))
            {
                has_children = 1;

                if (curr->state == TASK_ZOMBIE)
                {
                    if (status != NULL)
                        *status = curr->exit_code;

                    task_t *linker = ready_queue;

                    while (linker->next != curr)
                        linker = linker->next;

                    linker->next = curr->next;

                    if (ready_queue == curr)
                        ready_queue = curr->next;

                    int dead_pid = curr->pid;

                    if (curr->kernel_stack)
                        kfree((void *)(curr->kernel_stack - 4096));
                    kfree(curr);

                    return dead_pid;
                }
            }
            curr = curr->next;
        } while (curr != ready_queue);

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

task_t *pick_next_task()
{
    if(!ready_queue) return NULL;

    task_t *temp = current_task->next;
    do
    {
        if (temp->state == TASK_READY || temp->state==TASK_RUNNING)
            return temp;
        temp = temp->next;
    } while (temp != current_task);

    return current_task;
}
