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
                   }   }
                t = t->next;
            }
            while (t != ready_queue)
                ;
        }
        if (!found)
            kprinf("   (nobody holds it .... ref_count is stale)\n");

        kprintf("  VERDICT: ");
        if ((uint32_t)found == inode->ref_count && found > 0)
            kprintf("normal ... every reference accounted for\n\n");
        else if (found == 0 && inode->ref_count > 0)
            kprintf("LEAK ... ref_count=%u but no holder found,\n"
                    "           a sys_close was missed somewhere\n\n",
                    inode->ref_count);
        else if ((uint32_t)found != inode->ref_count)
            kprintf("MISMATCH ... %u holder(s) visible but ref_count=%u,\n"
                    "           %d reference(s) unaccounted for\n\n",
                    found, inode->ref_count,
                    (int)inode->ref_count - found);
        else
            kprintf("inode has zero references ... safe to free\n\n");
    }


void whyalive_task(uint32_t pid){
    if(!ready_queue) {
        kprintf("whyalive: no tasks running \n");
        return;
    }
    task_t *t=ready_queue;

    do{
        if (t->pid == pid)
        {
            kprintf("\n  WHAT:    task  pid %u (%s)\n", t->pid, t->name);
            kprintf("  CREATED: tick %u   parent pid %u\n",
                    t->start_time, t->parent ? t->parent->pid : 0);
            kprintf("  STATE:   %s\n", state_to_str(t->state));

            uint32_t alive = (t->destroy_time ? t->destroy_time : get_ticks())
                             - t->start_time;
            kprintf("  ALIVE:   %u ticks\n", alive);

            kprintf("  WHY:\n");
            if (t->state == TASK_ZOMBIE)
            {
                kprintf("    parent pid %u has not called waitpid(%u, ...)\n",
                        t->parent ? t->parent->pid : 0, t->pid);
                kprintf("  VERDICT: zombie awaiting reap ... becomes a permanent\n"
                        "           leak only if parent never calls waitpid\n\n");
            }
            else
            {
                kprintf("    task is %s and holding %d open fd(s)\n",
                        state_to_str(t->state),
                        ({ int c=0; for(int i=0;i<TASK_MAX_FDS;i++) if(t->fd_table[i]) c++; c; }));
                kprintf("  VERDICT: normal ... task is actively scheduled or runnable\n\n");
            }
            return;
        }
        t = t->next;
    } while (t != ready_queue);

    kprintf("whyalive: pid %u not found\n", pid);
}

