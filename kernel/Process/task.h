#ifndef TASK_H
#define TASK_H
#include "../include/vfs.h"

#include <stdint.h>

typedef enum
{
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_ZOMBIE

} task_state_t;

typedef struct task
{
    uint32_t pid;

    uint32_t esp;
    uint32_t ebp;
    uint32_t eip;
    file_t* fd_table[32];
    uint32_t cr3;

    uint32_t kernel_stack;
    dentry_t* cwd;

    task_state_t state;

    struct task *next;

} task_t;

extern task_t *current_task;
extern task_t *ready_queue;

void init_tasking();

task_t *create_process(void (*entry)(),uint32_t flags, uint32_t page_dir);

void schedule();

void sys_exit();

void sys_print(char *str);

#endif