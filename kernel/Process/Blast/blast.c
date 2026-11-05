#include <stddef.h>
#include "blast.h"
#include "../task.h"
#include "../../../Include/vfs.h"
#include "../../../Lib/kprintf.h"
#include "../../../Include/terminal.h"
#include "../../../Lib/string.h"
#include "../../Dev/dev.h"

/* Resolve a human-readable path for a held fd.
 * Prefer the file's own dentry (set by sys_open), then inode->dentry,
 * then walk parents. Fall back to a short name when no dentry exists
 * (e.g. early stdin/stdout/stderr that only hold a bare tty inode). */
static void path_for_file(file_t *f, char *out, size_t outlen)
{
    if (!out || outlen == 0)
        return;
    out[0] = '\0';
    if (!f)
    {
        strncpy(out, "(unknown)", outlen - 1);
        out[outlen - 1] = '\0';
        return;
    }

    dentry_t *d = f->dentry;
    if (!d && f->inode)
        d = f->inode->dentry;

    if (!d || !d->name)
    {
        /* stdio tty FDs are created without a dentry in task_init */
        if (f->inode)
        {
            /* Match registered devfs devices by inode pointer */
            if (devfs_get("tty") == f->inode)
            {
                strncpy(out, "/dev/tty", outlen - 1);
                out[outlen - 1] = '\0';
                return;
            }
        }
        strncpy(out, "(unknown)", outlen - 1);
        out[outlen - 1] = '\0';
        return;
    }

    /* Build path by walking parents into a temporary stack of names */
    const char *parts[32];
    int n = 0;
    dentry_t *cur = d;
    while (cur && n < 32)
    {
        if (cur->name && cur->name[0])
            parts[n++] = cur->name;
        cur = cur->parent;
    }

    if (n == 0)
    {
        strncpy(out, "(unknown)", outlen - 1);
        out[outlen - 1] = '\0';
        return;
    }

    /* parts[] is leaf..root; emit /root/.../leaf */
    size_t pos = 0;
    for (int i = n - 1; i >= 0; i--)
    {
        if (pos + 1 >= outlen)
            break;
        out[pos++] = '/';
        const char *p = parts[i];
        while (*p && pos + 1 < outlen)
            out[pos++] = *p++;
    }
    out[pos] = '\0';
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

        if (!f || !f->inode)
            continue;

        char pathbuf[128];
        path_for_file(f, pathbuf, sizeof(pathbuf));

        int other_hol = 0;
        t = ready_queue;
        do
        {
            for (int ofd = 0; ofd < TASK_MAX_FDS; ofd++)
            {
                /* Skip the fd we are reporting on the target task */
                if (t == target && ofd == fd)
                    continue;
                if (t->fd_table[ofd] && t->fd_table[ofd]->inode == f->inode)
                {
                    if (other_hol == 0)
                        kprintf("    %-16s shared with pid %u (%s) fd[%d]\n",
                                pathbuf, t->pid, t->name, ofd);
                    other_hol++;
                }
            }
            t = t->next;
        } while (t != ready_queue);

        uint32_t refs = f->inode->ref_count;
        if (other_hol > 0)
        {
            uint32_t after = refs > 0 ? refs - 1 : 0;
            kprintf("                     ref_count=%u, would drop to %u (safe, still held)\n",
                    refs, after);
            shared_safe++;
        }
        else
        {
            kprintf("    %-16s ref_count=%u, would drop to 0 and be reclaimed\n",
                    pathbuf, refs);
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