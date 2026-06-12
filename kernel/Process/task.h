#ifndef TASK_H
#define TASK_H

#include "../../Include/vfs.h"
#include "../Paging/isr.h"

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
    uint32_t cr3;
    uint32_t pid;
    task_state_t state;

    register_t regs;
    uint32_t kernel_stack;
    file_t *fd_table[32];

    dentry_t *cwd;

    struct task *next;
    struct task *parent;

    int started;
    int exit_code;
    uint32_t first_run;
} task_t;

extern task_t *current_task;
extern task_t *ready_queue;
extern int next_pid;

void init_tasking();

task_t *create_process(void (*entry)(), uint32_t flags, uint32_t page_dir);

task_t *task_create_kernel(void (*entry_point)());
task_t *task_create_user(void (*entry_point)());

task_t *pick_next_task(void);
void schedule();

void sys_exit(int status);

int do_fork(register_t *state_at_interuppt);

int sys_waitpid(int target_pid, int *status);

void task_add_ready(task_t* task);

#endif