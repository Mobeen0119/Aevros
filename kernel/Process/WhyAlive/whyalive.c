#include "whyalive.h"
#include "../task.h"
#include "../TaskLife/tasklife.h"
#include "../../../Include/vfs.h"
#include "../../Memory/KallocTracker/kalloc_tracker.h"
#include "../../../Lib/kprintf.h"

static const char *flags_to_str(uint32_t flags)
{
    if (flags & READ_WRITE)
        return "READ_WRITE";
    if (flags & WRITE_ONLY)
        return "WRITE_ONLY";
    if (flags & READ_ONLY)
        return "READ_ONLY";
    return "UNKNOWN";
}

static const char *state_to_str(task_state_t s)
{
    switch (s)
    {
    case TASK_READY:
        return "READY";
    case TASK_RUNNING:
        return "RUNNING";
    case TASK_BLOCKED:
        return "BLOCKED";
    case TASK_ZOMBIE:
        return "ZOMBIE";
    case TASK_SUSPENDED:
        return "SUSPENDED";
    default:
        return "UNKNOWN";
    }
}

void whyalive_inode_path(const char *path)
{
    if (!path)
    {
        kprintf("whyalive: missing path\n");
        return;
    }

    dentry_t *d = vfs_lookup(vfs_read, path);
    if (!d || !d->inode)
    {
        kprintf("whyalive: '%s' not found\n", path);
        return;
    }

    inode_t *inode = d->inode;

    kprintf("\n  WHAT:    inode  (file: %s)\n", path);
    kprintf("  CREATED: tick %u   by pid %u\n",
            inode->created_tick, inode->created_pid);
    kprintf("  ALIVE:   %u ticks\n", get_ticks() - inode->created_tick);
    kprintf("  ref_count = %u\n", inode->ref_count);
    kprintf("  WHY:\n");

    int found = 0;

    if (ready_queue)
    {
        task_t *t = ready_queue;

        do
        {
            for (int fd = 0; fd < TASK_MAX_FDS; fd++)
            {
                file_t *f = t->fd_table[fd];
                if (f && f->inode == inode)
                {
                    kprintf("    pid %-4u %-10s %s fd[%-2d] flags=%s offset=%u\n",
                            t->pid, t->name,
                            t->state == TASK_ZOMBIE ? "(ZOMBIE)" : "        ",
                            fd, flags_to_str(f->flags), f->offset);
                    found++;
                }
            }
            t = t->next;
        } while (t != ready_queue);
    }
    if (!found)
        kprintf("    no active holder found, ref_count may be stale\n");

    kprintf("  VERDICT : ");

    if ((uint32_t)found == inode->ref_count && found > 0)
    {
        kprintf("[OK]      every reference is accounted for\n\n");
    }
    else if (found == 0 && inode->ref_count > 0)
    {
        kprintf("[LEAK]    ref_count=%u but no holder exists\n"
                "          missing release detected, check close path\n\n",
                inode->ref_count);
    }
    else if ((uint32_t)found != inode->ref_count)
    {
        kprintf("[MISMATCH] %u holder(s) visible but ref_count=%u\n"
                "           %d reference(s) cannot be explained\n\n",
                found,
                inode->ref_count,
                (int)inode->ref_count - found);
    }
    else
    {
        kprintf("[FREE]    inode has zero references\n"
                "          safe to release\n\n");
    }
}

void whyalive_task(uint32_t pid)
{
    if (!ready_queue)
    {
        kprintf("whyalive: no tasks running \n");
        return;
    }
    task_t *t = ready_queue;

    do
    {
        if (t->pid == pid)
        {
            kprintf("\n");
            kprintf("  OBJECT  : task pid %u (%s)\n", t->pid, t->name);
            kprintf("  ORIGIN  : tick %u, parent pid %u\n",
                    t->start_time, t->parent ? t->parent->pid : 0);
            kprintf("  STATE   : %s\n", state_to_str(t->state));

            uint32_t alive =
                (t->destroy_time ? t->destroy_time : get_ticks()) - t->start_time;

            kprintf("  LIFETIME: %u ticks\n", alive);

            kprintf("  WHY     :\n");

            if (t->state == TASK_ZOMBIE)
            {
                kprintf("    parent pid %u has not called waitpid(%u, ...)\n",
                        t->parent ? t->parent->pid : 0,
                        t->pid);

                kprintf("  VERDICT : [ZOMBIE] task has exited but remains unreaped\n"
                        "            resources will be released after waitpid()\n\n");
            }
            else
            {
                int fd_count = 0;
                for (int i = 0; i < TASK_MAX_FDS; i++)
                    if (t->fd_table[i])
                        fd_count++;

                kprintf("    task is %s with %d open fd(s)\n",
                        state_to_str(t->state),
                        fd_count);

                kprintf("  VERDICT : [LIVE] task is active and resources are in use\n\n");
            }
            return;
        }
        t = t->next;
    } while (t != ready_queue);

    kprintf("whyalive: pid %u not found\n", pid);
}

void whyalive_alloc(void *ptr)
{
    if (!ptr)
    {
        kprintf("whyalive: missing pointer\n");
        return;
    }

    alloc_record_t *rec = tracker_find(ptr);

    if (!rec)
    {
        kprintf("whyalive: 0x%x is not a tracked live allocation.\n", (uint32_t)ptr);
        return;
    }
    kprintf("\n");
    kprintf("  OBJECT  : heap allocation (%u bytes)\n", rec->size);
    kprintf("  ORIGIN  : %s:%u in %s() by pid %u\n",
            rec->file, rec->line, rec->func, rec->pid);

    uint8_t pid_alive = 0;
    if (ready_queue)
    {
        task_t *t = ready_queue;
        do
        {
            if (t->pid == rec->pid && t->state != TASK_ZOMBIE)
            {
                pid_alive = 1;
                break;
            }
            t = t->next;
        } while (t != ready_queue);
    }

    kprintf("  WHY     : owning pid %u is %s\n",
            rec->pid, pid_alive ? "still alive" : "dead");

    kprintf("  VERDICT : ");
    if (pid_alive)
        kprintf("[OK]     allocation is in active use\n\n");
    else
        kprintf("[GHOST]  owner exited but allocation still exists\n"
                "          memory should have been released\n\n");
}
