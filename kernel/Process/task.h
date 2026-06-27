#ifndef TASK_H
#define TASK_H

#include "../../Include/vfs.h"
#include "../Paging/isr.h"

#include <stdint.h>

#define TASK_MAX_FDS 32
#define TASK_NAME_LEN 16
#define O_CLOEXEC 0x08
#define RLIMIT_CPU 0
#define RLIMIT_NOFILE 1
#define RLIMIT_STACK 2
#define RLIMIT_COUNT 3

#define NSIGNALS 32

typedef struct rlimit
{
    uint32_t rlim_cur;
    uint32_t rlim_max;
} rlimit_t;

#define RLIMIT_CPU 0
#define RLIMIT_NOFILE 1
#define RLIMIT_STACK 2
#define RLIMIT_COUNT 3
#define TASK_MAX_EVENTS 16

typedef enum
{
    EVT_CREATED,
    EVT_FIRST_RUN,
    EVT_EXEC,
    EVT_FD_OPEN,
    EVT_FD_CLOSE,
    EVT_FORKED,
    EVT_CHILD_DIED,
    EVT_FAULT,
    EVT_SIGNAL,
    EVT_BLOCKED,
    EVT_WOKE,
    EVT_EXITED,
    EVT_QUARANTINED

} task_event_type_t;

typedef struct
{
    task_event_type_t type;
    uint32_t tick, data;

} task_event_t;

typedef enum
{
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_ZOMBIE,
    TASK_SUSPENDED,
    TASK_QUARANTINED

} task_state_t;

typedef struct context
{
    uint32_t edi;
    uint32_t esi;
    uint32_t ebx;
    uint32_t ebp;
    uint32_t eip;
} context_t;

typedef struct task
{
    uint32_t cr3;
    uint32_t pid;
    task_state_t state;
    register_t regs;

    uint32_t kernel_stack;
    uint32_t kernel_stack_base;
    uint32_t context_esp;

    file_t *fd_table[TASK_MAX_FDS];
    dentry_t *cwd;
    struct task *next;
    struct task *all_next;
    struct task *parent;
    int started;

    int exit_code;
    uint32_t first_run;
    uint32_t signal_mask;

    uint32_t pending_signals;
    void (*signal_handlers[NSIGNALS])(int);
    rlimit_t rlimits[RLIMIT_COUNT];

    uint32_t user_time;
    uint32_t kernel_time;
    uint32_t start_time;
    task_event_t events[TASK_MAX_EVENTS];

    uint8_t event_count;
    uint32_t destroy_time;
    char name[TASK_NAME_LEN]; 
    uint8_t is_user;     

} task_t;

extern task_t *current_task;
extern task_t *ready_queue;
extern task_t *all_tasks;
extern int next_pid;

void init_tasking();

task_t *create_process(void (*entry)(), uint32_t flags, uint32_t page_dir, uint32_t user_stack_top);

task_t *task_create_kernel(void (*entry_point)());

task_t *task_create_user(void (*entry_point)());

task_t *pick_next_task(void);
void schedule();

void sys_exit(int status);
void dbg_hex32(uint32_t val);

int do_fork(register_t *state_at_interuppt);

int sys_waitpid(int target_pid, int *status);

void task_add_ready(task_t *task);
void task_register_all(task_t *task);

void task_wake(task_t *task);
uint32_t get_ticks(void);

void context_switch(uint32_t *old_esp, uint32_t new_esp);
void trap_return(void);
uint32_t read_eip(void);
uint32_t build_initial_stack(uint8_t *stack_base, uint32_t entry_point, uint32_t cs, uint32_t ss, uint32_t user_esp);

#endif