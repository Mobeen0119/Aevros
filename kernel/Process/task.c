#include "task.h"
#include "pmm.h"
#include "Paging/paging.c"
#include "../Include/vfs.h"
#include "../io.c"
#include "../include/screen.h"
#include "../Dev/dev.h"
#include "../Memory/kheap.c"
#include "../CPU/tss.h"
#include "userspace.h"
#define Temp_p_vir_addr 0xFFC00000

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

    asm volatile("mov %%esp, %0" : "=r"(current_task->regs.esp));
    asm volatile("mov %%ebp, %0" : "=r"(current_task->regs.ebp));

    current_task->cr3 = read_cr3();
    current_task->regs.eip = read_eip();
    current_task->kernel_stack = current_task->regs.esp;

    current_task->next = current_task;
    ready_queue = current_task;
}

task_t *create_process(void (*entry_point)(), uint32_t flags, uint32_t page_dir)
{

    task_t *new_task = (task_t *)kmalloc(sizeof(task_t));
    if (!new_task)
        return 0;

    memset(new_task, 0, sizeof(task_t));
    new_task->pid = next_pid++;
    new_task->state = TASK_READY;

    uint8_t *stack_base = kmalloc(4096);
    if (!stack_base)
        return 0;

    uint32_t stack_top = (uint32_t)stack_base + 4096;

    uint32_t *sp = (uint32_t *)stack_top;
    *(--sp) = (uint32_t)entry_point;

    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;

    if (page_dir)
        new_task->cr3 = page_dir;
    else
        new_task->cr3 = read_cr3();

    new_task->regs.ebp = new_task->regs.esp = (uint32_t)sp;
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
    if (!current_task)
        return;

    if (!current_task->started)
    {
        current_task->started = 1;

        asm volatile("mov %0,%%cr3" ::"r"(current_task->cr3));
        enter_usermode(current_task->regs.eip,
                       current_task->regs.useresp);
    }

    task_t *prev = current_task;
    current_task = current_task->next;

    current_task->state = TASK_RUNNING;

    if (prev->state == TASK_RUNNING)
        prev->state = TASK_READY;

    tss.esp0 = current_task->kernel_stack;

    switch_current_task(prev, current_task);
}

void pit_init(uint32_t frequency)
{

    uint32_t divisor = 1193182 / frequency;

    outb(0x43, 0x36);

    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)(divisor >> 8) & 0xFF);
}

void sys_print(char *user_string)
{
    if (!user_string)
        return;

    for (int i = 0; user_string[i]; i++)
    {
        kput_char(user_string[i]);
    }
}

void sys_exit(int status)
{
    task_t *dead = current_task;
    if (!dead)
        return;

    dead->exit_code = status;

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

    current_task = ready_queue;
    tss.esp0 = current_task->kernel_stack;

    dead->state = TASK_ZOMBIE;

    switch_current_task(dead, current_task);

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