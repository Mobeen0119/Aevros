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

task_t *current = 0, *ready_queue = 0;

int next_pid = 0;

extern void switch_current_task(task_t *prev, task_t *next);
extern void read_eip();

static inline uint32_t read_cr3()
{
    uint32_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

void init_tasking()
{
    current = (task_t *)kmalloc(sizeof(task_t));
    if (!current)
        return;

    memset(current, 0, sizeof(task_t));

    current->pid = next_pid++;
    current->state = TASK_RUNNING;
    current->cwd = vfs_root;

    memset(current->fd_table, 0, sizeof(current->fd_table));

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

    current->fd_table[0] = stdin;
    current->fd_table[1] = stdout;
    current->fd_table[2] = stderr;

    asm volatile("mov %%regs.esp, %0" : "=r"(current->regs.esp));
    asm volatile("mov %%regs.ebp, %0" : "=r"(current->regs.ebp));

    current->cr3 = read_cr3();
    current->regs.eip = read_eip();
    current->kernel_stack = current->regs.esp;

    current->next = current;
    ready_queue = current;
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
    if (!current)
        return;

    if (!current_task->started)
    {
        current_task->started = 1;

        asm volatile("mov %0,%%cr3" ::"r"(current_task->cr3));
        enter_usermode(current_task->regs.regs.eip,
                       current_task->regs.userregs.esp);
    }

    task_t *prev = current;
    current = current->next;

    current->state = TASK_RUNNING;

    if (prev->state == TASK_RUNNING)
        prev->state = TASK_READY;

    tss.esp0 = current->kernel_stack;

    switch_current_task(prev, current);
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

void sys_exit()
{
    if (!current)
        return;

    for (int i = 0; i < 32; i++)
    {
        if (current->fd_table[i])
        {
            sys_close(i);
        }
    }

    task_t *dead = current;

    task_t *temp = current;
    while (temp->next != current)
        temp = temp->next;

    temp->next = current->next;
    current = current->next;
    dead->state = TASK_ZOMBIE;

    switch_current_task(dead, current);

    __builtin_unreachable();
}
