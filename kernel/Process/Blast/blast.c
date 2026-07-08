#include <stddef.h>
#include "blast.h"
#include "../task.h"
#include "../../../Include/vfs.h"
#include "../../../Lib/kprintf.h"
#include "../../../Include/terminal.h"
#include "../../../Lib/string.h"

static const char *inode_name(inode_t *inode)
{
    if (inode->dentry && inode->dentry->name)
        return inode->dentry->name;
    return "(unknown)";
}

void blast_radius(uint32_t pid)
{
    if (!ready_queue)
    {
        set_color(VGA_YELLOW, VGA_BLACK);
        kprintf("blast: no tasks running\n");
        reset_color();
        return;
    }
    task_t *target = NULL;
    task_t *t = ready_queue;

    do
    {
        if (t->pid == pid)
        {
            target = t;
            break;
        }
        t = t->next;
    } while (t != ready_queue);

    if (!target)
    {
        set_color(VGA_RED, VGA_BLACK);
        kprintf("blast: pid %u not found\n", pid);
        reset_color();
        return;
    }

    print_heading("BLAST RADIUS");
    kprintf("  if pid %u (%s) disappears:\n\n",
            target->pid, target->name);

    int orphans = 0, would_free = 0, shared_safe = 0;

    kprintf("  CHILDREN (would become orphans):\n");
    t = ready_queue;
    do
    {
        if (t->parent == target && t->state != TASK_ZOMBIE)
        {
            kprintf("    pid %-4u (%s)\n", t->pid, t->name);
            orphans++;
        }
        t = t->next;
    } while (t != ready_queue);
    if (!orphans)
        kprintf("    (none)\n");

    kprintf("\n FILES HELD: \n");
    for (int fd = 0; fd < TASK_MAX_FDS; fd++)
    {
        file_t *f = target->fd_table[fd];

        if (!f)
            continue;
        int other_hol = 0;
        t = ready_queue;
        do
        {
            for (int ofd = 0; ofd < TASK_MAX_FDS; ofd++)
            {
                if (t->fd_table[ofd] && t->fd_table[ofd]->inode == f->inode)
                {
                    if (other_hol == 0)
                        kprintf("    %-16s shared with pid %u (%s) fd[%d]\n",
                                inode_name(f->inode), t->pid, t->name, ofd);
                    other_hol++;
                }
            }
            t = t->next;
        } while (t != ready_queue);

        if (other_hol > 0)
        {
            kprintf("                     ref_count=%u, would drop to %u (safe, still held)\n",
                    f->inode->ref_count, f->inode->ref_count - 1);
            shared_safe++;
        }
        else
        {
            kprintf("    %-16s ref_count=%u, would drop to 0 and be reclaimed\n",
                    inode_name(f->inode), f->inode->ref_count);
            would_free++;
        }
    }
    if (!would_free && !shared_safe)
        kprintf("    (none)\n");

    const char *impact = (orphans == 0 && would_free == 0) ? "LOW" : (orphans + would_free <= 2) ? "MEDIUM"
                                                                                                 : "HIGH";

    set_color(strcmp(impact, "LOW") == 0 ? VGA_GREEN : strcmp(impact, "MEDIUM") == 0 ? VGA_YELLOW : VGA_RED, VGA_BLACK);
    kprintf("\n IMPACT : %s\n", impact);
    kprintf("   %d orphaned child(ren), %d file(s) would free, %d file(s) remain safely shared\n\n",
            orphans, would_free, shared_safe);
    reset_color();
}
