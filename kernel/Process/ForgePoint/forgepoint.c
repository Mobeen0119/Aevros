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

