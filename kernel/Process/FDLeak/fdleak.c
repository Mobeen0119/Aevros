#include "fdleak.h"
#include "../task.h"
#include "../TaskLife/tasklife.h"
#include "../../../Include/vfs.h"
#include "../../../Lib/kprintf.h"

#define FDLEAK_AGE_THRESHOLD 100

static const char *path_for_inode(inode_t *inode)
{
    if (inode->dentry && inode->dentry->name)
        return inode->dentry->name;
    return "(unknown)";
}

static uint32_t fd_open_tick(task_t *t, int fd)
{
    for (int i = t->event_count - 1; i >= 0; i--)
    {
        if (t->events[i].type == EVT_FD_OPEN && t->events[i].data == (uint32_t)fd)
            return t->events[i].tick;
    }
    return t->start_time;
}

void fdleak_scan(void)
{
    if (!ready_queue)
    {
        kprintf("fdleak: no tasks running...First Run\n");
        return;
    }

    kprintf("\n fdleak scan: \n\n");

    int leaks = 0, suspicious = 0;
    uint32_t now = get_ticks();

    task_t *t = ready_queue;

    do
    {
        int task_header_shown = 0;

        for (int fd = 0; fd < TASK_MAX_FDS; fd++)
        {
            file_t *f = t->fd_table[fd];

            if (!f)
                continue;

            if (!task_header_shown)
            {
                kprintf("  pid %u (%s)%s\n",
                        t->pid, t->name,
                        t->state == TASK_ZOMBIE ? "  (ZOMBIE)" : "");
                task_header_shown = 1;
            }
            uint32_t opened_tick = fd_open_tick(t, fd);
            uint32_t age = now - opened_tick;

            if (t->state == TASK_ZOMBIE)
            {
                kprintf("    fd[%-2d]  %-14s opened tick %-5u age %-5u  "
                        "<- LEAK, dead task still holds this fd\n",
                        fd, path_for_inode(f->inode), opened_tick, age);
                leaks++;
                continue;
            }

            if (f->offset == 0 && age > FDLEAK_AGE_THRESHOLD)
            {
                kprintf("    fd[%-2d]  %-14s opened tick %-5u age %-5u  "
                        "<- unused since open\n",
                        fd, path_for_inode(f->inode), opened_tick, age);
                suspicious++;
                continue;
            }

            for (int other = fd + 1; other < TASK_MAX_FDS; other++)
            {
                if (t->fd_table[other] && t->fd_table[other]->inode == f->inode)
                {
                    kprintf("    fd[%-2d]  %-14s opened tick %-5u age %-5u  "
                            "<- duplicate of fd[%d], same file\n",
                            fd, path_for_inode(f->inode), opened_tick, age, other);
                    suspicious++;
                    break;
                }
            }
        }
        t = t->next;
    } while (t != ready_queue);
  
    kprintf("\n Summary: %d confirmed leak(s), %d suspicious fd(s)\n\n", leaks, suspicious);
}