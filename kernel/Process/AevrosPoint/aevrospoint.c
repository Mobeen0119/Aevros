#include "aevrospoint.h"
#include "../TaskLife/tasklife.h"
#include "../../../Include/vfs.h"
#include "../../Memory/kheap.h"
#include "../../../Lib/kprintf.h"
#include "../../../Lib/string.h"
#include "../../Paging/paging.h"
#include "../../Memory/pmm.h"
#include "../../../Include/terminal.h"

#define AEVROSPOINT_MAGIC 0x46475054
#define FP_MAX_PAGES 256

typedef struct
{
    uint32_t magic;
    char name[TASK_NAME_LEN];
    uint32_t saved_tick;

    uint8_t is_user;
    uint16_t cs_saved;
    uint16_t ss_saved;

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

    uint32_t page_count;
    uint32_t page_vaddr[FP_MAX_PAGES];
    uint32_t page_flags[FP_MAX_PAGES];
    uint8_t page_data[FP_MAX_PAGES][4096];
} fgpt_snapshot_t;

static void build_path(const char *name, char *out, uint32_t outlen)
{
    strncpy(out, "/checkpoint_", outlen);

    uint32_t base_len = strlen(out);

    strncpy(out + base_len, name, outlen - base_len - 5);
    strncpy(out + strlen(out), ".ckpt", outlen - strlen(out));
}

static task_t *find_task_by_name(const char *name)
{
    if (!ready_queue)
        return NULL;

    task_t *t = ready_queue;

    do
    {
        if (strcmp(t->name, name) == 0)
            return t;

        t = t->next;
    } while (t != ready_queue);

    return NULL;
}

static int snapshot_user_pages(uint32_t cr3, fgpt_snapshot_t *snap)
{
    snap->page_count = 0;

    asm volatile("cli");
    map_page(TEMP_PD_VIRT, cr3, PAGE_PRESENT | PAGE_WRITE);
    uint32_t *tar_pd = (uint32_t *)TEMP_PD_VIRT;

    for (int pd = 10; pd < 768; pd++)
    {
        if (!(tar_pd[pd] & PAGE_PRESENT))
            continue;

        uint32_t pt_phy = tar_pd[pd] & 0xFFFFF000;
        map_page(TEMP_PT_VIRT, pt_phy, PAGE_PRESENT | PAGE_WRITE);
        uint32_t *tar_pt = (uint32_t *)TEMP_PT_VIRT;

        for (int pt = 0; pt < 1024; pt++)
        {
            if (!(tar_pt[pt] & PAGE_PRESENT))
                continue;

            if (snap->page_count >= FP_MAX_PAGES)
            {
                unmap(TEMP_PT_VIRT);
                unmap(TEMP_PD_VIRT);
                asm volatile("sti");
                kprintf("checkpoint: process exceeds %u page cap, aborting save\n", FP_MAX_PAGES);
                return -1;
            }

            uint32_t phys = tar_pt[pt] & 0xFFFFF000;
            uint32_t flags = tar_pt[pt] & 0xFFF;
            uint32_t vaddr = (pd << 22) | (pt << 12);

            uint32_t index = snap->page_count;
            snap->page_vaddr[index] = vaddr;
            snap->page_flags[index] = flags;

            map_page(TEMP_SRC_PAGE, phys, PAGE_PRESENT | PAGE_WRITE);
            memcpy(snap->page_data[index], (void *)TEMP_SRC_PAGE, 4096);
            unmap(TEMP_SRC_PAGE);

            snap->page_count++;
        }

        unmap(TEMP_PT_VIRT);
    }
    unmap(TEMP_PD_VIRT);
    asm volatile("sti");
    return 0;
}

static uint32_t restore_user_pages(fgpt_snapshot_t *snap)
{
    uint32_t *new_pd = (uint32_t *)alloc_page_aligned();

    if (!new_pd)
        return 0;

    memset(new_pd, 0, 4096);

    uint32_t *current_pd = (uint32_t *)PAGE_RECURSIVE;

    for (int i = 0; i < 10; i++)
    {
        new_pd[i] = current_pd[i];
    }

    for (int i = 768; i < 1024; i++)
    {
        new_pd[i] = current_pd[i];
    }

    uint32_t new_pd_phys = (uint32_t)new_pd;

    new_pd[1023] = new_pd_phys | PAGE_PRESENT | PAGE_WRITE;

    for (uint32_t i = 0; i < snap->page_count; i++)
    {
        uint32_t phys = pmm_alloc();
        if (!phys)
        {
            kprintf("checkpoint: out of memory restoring page %u/%u\n",
                    i, snap->page_count);
            return 0;
        }
        map_page(TEMP_DST_PAGE, phys, PAGE_PRESENT | PAGE_WRITE);
        memcpy((void *)TEMP_DST_PAGE, snap->page_data[i], 4096);
        unmap(TEMP_DST_PAGE);

        if (!map_page_in_directory(new_pd_phys, snap->page_vaddr[i], phys, snap->page_flags[i]))
        {
            kprintf("checkpoint: failed mapping restored page %u\n", i);
            return 0;
        }
    }
    return new_pd_phys;
}

int aevrospoint_save(const char *name)
{
    if (!name)
        return VFS_ERR;

    task_t *target = find_task_by_name(name);
    if (!target)
    {
        kprintf("checkpoint: task '%s' not found\n", name);
        return VFS_ERR;
    }
    if (target->kernel_stack_base == 0)
    {
        kprintf("checkpoint: task '%s' has no saveable kernel stack\n", name);
        return VFS_ERR;
    }

    fgpt_snapshot_t *snap = kmalloc(sizeof(fgpt_snapshot_t));
    if (!snap)
        return VFS_ERR;

    memset(snap, 0, sizeof(fgpt_snapshot_t));

    snap->magic = AEVROSPOINT_MAGIC;
    strncpy(snap->name, target->name, TASK_NAME_LEN);

    snap->saved_tick = get_ticks();

    snap->is_user = (target->regs.cs & 0x3) ? 1 : 0;
    snap->cs_saved = target->regs.cs;
    snap->ss_saved = target->regs.ss;

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

    if (snap->is_user)
    {
        if (snapshot_user_pages(target->cr3, snap) != 0)
        {
            kfree(snap);
            return VFS_ERR;
        }
    }
    else
    {
        snap->page_count = 0;
    }

    char path[64];
    build_path(name, path, sizeof(path));

    int fd = sys_open(path, READ_WRITE | CREAT);

    if (fd < 0)
    {
        kfree(snap);
        kprintf("checkpoint: failed to open %s \n", path);
        return VFS_ERR;
    }

    int written = sys_write(fd, (uint8_t *)snap, sizeof(fgpt_snapshot_t));

    sys_close(fd);

    int ok = (written == (int)sizeof(fgpt_snapshot_t));

    if (!ok)
    {
        kprintf("checkpoint: short write, snapshot may be corrupt\n");
        return VFS_ERR;
    }

    kprintf("checkpoint: saved '%s' -> %s  (%d bytes, %u user pages, %s)\n",
            name, path, written, snap->page_count, snap->is_user ? "user" : "kernel");
    kfree(snap);

    return VFS_OK;
}

int aevrospoint_restore(const char *name)
{
    if (!name)
        return VFS_ERR;

    char path[64];
    build_path(name, path, sizeof(path));

    int fd = sys_open(path, READ_ONLY);
    if (fd < 0)
    {
        kprintf("checkpoint: no snapshot for '%s' \n", name);
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

    if (got != (int)sizeof(fgpt_snapshot_t) || snap->magic != AEVROSPOINT_MAGIC)
    {
        kprintf("checkpoint: snapshot '%s' invalid or corrupt \n", name);
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

    task->regs.ebp = snap->regs.ebp;

    task->regs.cs = snap->cs_saved;
    task->regs.ss = snap->ss_saved;

    if (snap->is_user)
    {
        uint32_t new_cr3 = restore_user_pages(snap);
        if (!new_cr3)
        {
            kprintf("checkpoint: failed to rebuild address space for '%s'\n", name);
            kfree(new_stack);
            kfree(task);
            kfree(snap);
            return VFS_ERR;
        }
        task->cr3 = new_cr3;
    }
    else
    {
        asm volatile("mov %%cr3, %0" : "=r"(task->cr3));
    }

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

    kprintf("checkpoint: restored '%s' as pid %u (%u user pages, %s, saved tick %u, now tick %u)\n",
            snap->name, task->pid, snap->page_count,
            snap->is_user ? "user" : "kernel",
            snap->saved_tick, get_ticks());

    kfree(snap);
    task_add_ready(task);

    return task->pid;
}

void aevrospoint_list(void)
{
    int fd = sys_open("/", READ_ONLY);
    if (fd < 0)
    {
        kprintf("checkpoint: cannot open root\n");
        return;
    }

    dirent_t entry;
    int found = 0;

    set_color(VGA_CYAN, VGA_BLACK);
    kprintf("\n CHECKPOINTS \n");
    kprintf("  -----------------\n");
    reset_color();

    while (sys_readdir(fd, &entry) == 1)
    {
        if (strncmp(entry.name, "checkpoint_", 11) == 0)
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
