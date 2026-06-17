#include "forgepoint.h"
#include "../TaskLife/tasklife.h"
#include "../../../Include/vfs.h"
#include "../../Memory/kheap.h"
#include "../../../Lib/kprintf.h"
#include "../../../Lib/string.h"

#define FORGEPOINT_MAGIC 0x46475054

typedef struct
{
    uint32_t magic;
    char name[TASK_NAME_LEN];
    uint32_t saved_tick;

    register_t regs;

    uint32_t stack_used;
    uint8_t stack_data[4096];

    uint32_t fd_flags[TASK_MAX_FDS];
    uint32_t fd_offset[TASK_MAX_FDS];
    uint8_t fd_present[TASK_MAX_FDS];

    task_event_t events[TASK_MAX_EVENTS];
    uint8_t event_count;
    uint32_t start_time;

    uint32_t user_time;
    uint32_t kernel_time;
} fgpt_snapshot_t;

static void build_path(const char *name, char *out, uint32_t outlen)
{
    strncpy(out, "/forgepoint_", outlen);

    uint32_t base_len = strlen(out);

    strncpy(out + base_len, name, outlen - base_len - 4);
    strncpy(out + strlen(out), ".fgp", outlen - strlen(out));
}

int forgepoint_save(const char *name)
{
    if (!name || !current_task)
        return VFS_ERR;

    task_t *t = current_task;

    task_t *target = NULL;
    task_t *walk = ready_queue;

    if (walk)
    {
        do
        {
            if (strcmp(walk->name, name) == 0)
            {
                target = walk;
                break;
            }
            walk = walk->next;

        } while (walk != ready_queue);
    }

    if (!target)
    {
        kprintf("forgepoint: task '%s' not found\n", name);
        return VFS_ERR;
    }

    if (target->kernel_stack_base == 0)
    {
        kprintf("forgepoint: task '%s' has no saveable kernel stack\n", name);
        return VFS_ERR;
    }

    fgpt_snapshot_t *snap = kmalloc(sizeof(fgpt_snapshot_t));
    if (!snap)
        return VFS_ERR;

    memset(snap, 0, sizeof(fgpt_snapshot_t));

    snap->magic = FORGEPOINT_MAGIC;
    strncpy(snap->name, target->name, TASK_NAME_LEN);

    snap->saved_tick = get_ticks();

    snap->regs = target->regs;

    uint32_t stack_top = target->kernel_stack;
    uint32_t stack_base = target->kernel_stack_base;
    uint32_t used = stack_top - target->regs.esp;

    if (used > 4096)
        used = 4096;

    snap->stack_used = used;

    memcpy(snap->stack_data, (void *)target->regs.esp, used);

    for (int i = 0; i < TASK_MAX_FDS; i++)
    {
        if (target->fd_table[i])
        {
            snap->fd_present[i] = 1;
            snap->fd_flags[i] = target->fd_table[i]->flags;
            snap->fd_offset[i] = target->fd_table[i]->offset;
        }
    }

    snap->event_count = target->event_count;
    memcpy(snap->events, target->events, sizeof(target->events));

    snap->start_time = target->start_time;
    snap->user_time = target->user_time;
    snap->kernel_time = target->kernel_time;

    char path[64];
    build_path(name, path, sizeof(path));

    int fd = sys_open(path, READ_WRITE | CREAT);

    if (fd < 0)
    {
        kfree(snap);
        kprintf("forgepoint: failed to open %s \n", path);
        return VFS_ERR;
    }

    int written = sys_write(fd, (uint8_t *)snap, sizeof(fgpt_snapshot_t));

    sys_close(fd);
    kfree(snap);

    if (written != sizeof(fgpt_snapshot_t))
    {
        kprintf("forgepoint: short write, snapshot may be corrupt\n");
        return VFS_ERR;
    }

    kprinf("forgepoint: saved '%s' -> %s (%u bytes)\n", name, path, written);

    return VFS_OK;
}

int forgepoint_restore(const char *name)
{
    if (!name)
        return VFS_ERR;

    char path[64];
    build_path(name, path, sizeof(path));

    int fd = sys_open(path, READ_ONLY);
    if (fd < 0)
    {
        kprintf("forgepoint: no snapshot of '%s' \n", name);
        return VFS_ERR;
    }

    fgpt_snapshot_t *snap = kmalloc(sizeof(fgpt_snapshot_t));

    if (!snap)
    {
        sys_close(fd);
        return VFS_ERR;
    }

    int got = sys_read(fd, (uint8_t *)snap, sizeof(fgpt_snapshot_t));
    sys_close(fd);

    if (got != sizeof(fgpt_snapshot_t) || snap->magic != FORGEPOINT_MAGIC)
    {
        kprintf("forgepoint: snapshot '%s' invalid or corrupt \n", name);
        kfree(snap);
        return VFS_ERR;
    }
    task_t *task = kmalloc(sizeof(task_t));
    if (!task)
    {
        kfree(snap);
        return VFS_ERR;
    }

    memset(task, 0, sizeof(task_t));

    uint8_t *new_stack = kmalloc(4096);
    if (!new_stack)
    {
        kfree(task);
        kfree(snap);
        return VFS_ERR;
    }
    task->pid = next_pid++;
    task->state = TASK_READY;
    task->parent = current_task;

    strncpy(task->name, snap->name, TASK_NAME_LEN);

    task->cr3 = current_task->cr3;

    uint32_t stack_top = (uint32_t)new_stack + 4096;
    task->kernel_stack = stack_top;
    task->kernel_stack_base = (uint32_t)new_stack;

    task->regs = snap->regs;
    task->regs.esp = stack_top - snap->stack_used;
    memcpy((void *)task->regs.esp, snap->stack_data, snap->stack_used);

    uint32_t old_esp = stack_top - snap->stack_used;
    (void)old_esp;

    task->regs.ebp = snap->regs.ebp;

    for (int i = 0; i < TASK_MAX_FDS; i++)
    {
        if (snap->fd_present[i] && current_task->fd_table[i])
        {
            task->fd_table[i] = current_task->fd_table[i];
            task->fd_table[i]->inode->ref_count++;
            task->fd_table[i]->offset = snap->fd_offset[i];
        }
    }

    task->event_count = snap->event_count;
    memcpy(task->events, snap->events, sizeof(task->events));
    task->start_time = snap->start_time;

    task->user_time = snap->user_time;
    task->kernel_time = snap->kernel_time;

    task_log_event(task, EVT_CREATED, 0);
    kfree(snap);
    task_add_ready(task);

    kprintf("forgepoint: restored '%s' as pid %u (was tick %u, now tick %u)\n",
            snap->name, task->pid, snap->saved_tick, get_ticks());

    return task->pid;
}

void forgepoint_list(void)
{
    int fd = sys_open("/", READ_ONLY);
    if (fd < 0)
    {
        kprintf("forgepoint: cannot open root\n");
        return;
    }

    dirent_t entry;
    int found = 0;

    kprintf("\n FORGEPOINTS \n");
    kprintf("  ─────────────────\n");

    while (sys_readdir(fd, &entry) == 1)
    {
        if (strncmp(entry.name, "forgepoint_", 11) == 0)
        {
            kprintf("   %s \n", entry.name);
            found++;
        }
    }

    if (!found)
        kprintf("    (none saved yet)\n");

    sys_close(fd);
    kprintf("\n");
}
