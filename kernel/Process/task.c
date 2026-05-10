#include "task.h"
#include "pmm.h"
#include "Paging/paging.c"
#include "../VFS/vfs.c"
#include "../io.c"
#include "../include/screen.h"
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

    current->pid = next_pid++;
    current->state = TASK_RUNNING;
    memset(current->fd_table,0,sizeof(current->fd_table));

    asm volatile("mov %%esp, %0" : "=r"(current->esp));
    asm volatile("mov %%ebp, %0" : "=r"(current->ebp));

    current->cr3 = read_cr3();
    current->eip = read_eip();
    current->kernel_stack = current->esp;
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

    new_task->ebp = new_task->esp = (uint32_t)sp;
    new_task->eip = (uint32_t)entry_point;
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

    task_t *prev = current;
    current = current->next;
    current->state = TASK_RUNNING;
    if(prev->state==TASK_RUNNING)
        prev->state=TASK_READY;

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

    for (int i = 0; i < 1024; i++)
    {
        char c = user_string[i];
        if (c == '\0')
            break;
        kput_char(c);
    }
}

void sys_exit()
{
    if (!current)
        return;

    for(int i=0;i<32;i++){
        if(current->fd_table[i]){
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

    __buitin_unreachable();
}