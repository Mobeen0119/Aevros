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
#define STACK_PAGE_SIZE 4096

typedef struct
{
    uint32_t magic;
    char name[TASK_NAME_LEN];
    uint32_t saved_tick;
    uint8_t is_user;
    uint16_t cs_saved;
    uint16_t ss_saved;
    register_t regs;
    uint32_t stack_base_orig;
    uint32_t context_esp_offset;
    uint32_t stack_used;
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
} fgpt_header_t;

typedef struct
{
    uint32_t magic;
    char name[TASK_NAME_LEN];
    uint32_t saved_tick;
    uint8_t is_user;
    uint16_t cs_saved;
    uint16_t ss_saved;
    register_t regs;
    uint32_t stack_base_orig;
    uint32_t context_esp_offset;
    uint32_t stack_used;
    uint8_t stack_data[STACK_PAGE_SIZE];
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
    
    uint8_t (*page_data)[4096];
} fgpt_snapshot_t;

static fgpt_snapshot_t *work_snapshot = NULL;
static task_t *worker_task = NULL;
static task_t *waiter_task = NULL;

static int do_actual_save_to_file(task_t *target);
static void build_path(const char *name, char *out, uint32_t outlen);
static task_t *find_task_by_name(const char *name);
static int snapshot_user_pages(uint32_t cr3, fgpt_snapshot_t *snap);
static uint32_t restore_user_pages(fgpt_snapshot_t *snap);
static void relocate_stack_pointers(uint8_t *stack, uint32_t old_base, uint32_t new_base);
static int write_snapshot_to_disk(fgpt_snapshot_t *snap);

static void build_path(const char *name, char *out, uint32_t outlen)
{
    strncpy(out, "/checkpoint_", outlen);
    uint32_t base_len = strlen(out);
    strncpy(out + base_len, name, outlen - base_len - 5);
    strncpy(out + strlen(out), ".ckpt", outlen - strlen(out));
}

static task_t *find_task_by_name(const char *name)
{
    if (!ready_queue) return NULL;
    task_t *t = ready_queue;
    do {
        if (strcmp(t->name, name) == 0) return t;
        t = t->next;
    } while (t != ready_queue);
    return NULL;
}

static void relocate_stack_pointers(uint8_t *stack, uint32_t old_base, uint32_t new_base)
{
    if (old_base == new_base) return;
    uint32_t *words = (uint32_t *)stack;
    for (uint32_t i = 0; i < STACK_PAGE_SIZE / 4; i++)
    {
        uint32_t v = words[i];
        if (v >= old_base && v < old_base + STACK_PAGE_SIZE)
            words[i] = v - old_base + new_base;
    }
}

static int snapshot_user_pages(uint32_t cr3, fgpt_snapshot_t *snap)
{
    snap->page_count = 0;
    snap->page_data = NULL;
    asm volatile("cli");
    map_page(TEMP_PD_VIRT, cr3, PAGE_PRESENT | PAGE_WRITE);
    uint32_t *tar_pd = (uint32_t *)TEMP_PD_VIRT;

    uint32_t count = 0;
    for (int pd = 10; pd < 768; pd++)
    {
        if (!(tar_pd[pd] & PAGE_PRESENT)) continue;
        uint32_t pt_phy = tar_pd[pd] & 0xFFFFF000;
        map_page(TEMP_PT_VIRT, pt_phy, PAGE_PRESENT | PAGE_WRITE);
        uint32_t *tar_pt = (uint32_t *)TEMP_PT_VIRT;
        for (int pt = 0; pt < 1024; pt++)
            if (tar_pt[pt] & PAGE_PRESENT) count++;
        unmap(TEMP_PT_VIRT);
    }

    if (count == 0)
    {
        unmap(TEMP_PD_VIRT);
        asm volatile("sti");
        return 0;
    }
    if (count > FP_MAX_PAGES)
    {
        unmap(TEMP_PD_VIRT);
        asm volatile("sti");
        kprintf("checkpoint: process exceeds %u page cap, aborting save\n", FP_MAX_PAGES);
        return -1;
    }

    uint8_t (*page_data)[4096] = kmalloc(count * 4096);
    if (!page_data)
    {
        unmap(TEMP_PD_VIRT);
        asm volatile("sti");
        kprintf("checkpoint: out of memory allocating %u KB for page data\n", count * 4);
        return -1;
    }

    uint32_t index = 0;
    for (int pd = 10; pd < 768; pd++)
    {
        if (!(tar_pd[pd] & PAGE_PRESENT)) continue;
        uint32_t pt_phy = tar_pd[pd] & 0xFFFFF000;
        map_page(TEMP_PT_VIRT, pt_phy, PAGE_PRESENT | PAGE_WRITE);
        uint32_t *tar_pt = (uint32_t *)TEMP_PT_VIRT;
        for (int pt = 0; pt < 1024; pt++)
        {
            if (!(tar_pt[pt] & PAGE_PRESENT)) continue;
            uint32_t phys = tar_pt[pt] & 0xFFFFF000;
            uint32_t flags = tar_pt[pt] & 0xFFF;
            uint32_t vaddr = (pd << 22) | (pt << 12);
            snap->page_vaddr[index] = vaddr;
            snap->page_flags[index] = flags;
            map_page(TEMP_SRC_PAGE, phys, PAGE_PRESENT | PAGE_WRITE);
            memcpy(page_data[index], (void *)TEMP_SRC_PAGE, 4096);
            unmap(TEMP_SRC_PAGE);
            index++;
        }
        unmap(TEMP_PT_VIRT);
    }
    unmap(TEMP_PD_VIRT);
    asm volatile("sti");
    snap->page_data = page_data;
    snap->page_count = count;
    return 0;
}

static uint32_t restore_user_pages(fgpt_snapshot_t *snap)
{
    uint32_t *new_pd = (uint32_t *)alloc_page_aligned();
    if (!new_pd) return 0;
    memset(new_pd, 0, 4096);
    uint32_t *current_pd = (uint32_t *)PAGE_RECURSIVE;
    for (int i = 0; i < 10; i++) new_pd[i] = current_pd[i];
    for (int i = 768; i < 1024; i++) new_pd[i] = current_pd[i];
    uint32_t new_pd_phys = (uint32_t)new_pd;
    new_pd[1023] = new_pd_phys | PAGE_PRESENT | PAGE_WRITE;
    for (uint32_t i = 0; i < snap->page_count; i++)
    {
        uint32_t phys = pmm_alloc();
        if (!phys)
        {
            kprintf("checkpoint: out of memory restoring page %u/%u\n", i, snap->page_count);
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

static int write_snapshot_to_disk(fgpt_snapshot_t *snap)
{
    char path[64];
    build_path(snap->name, path, sizeof(path));
    int fd = sys_open(path, READ_WRITE | CREAT);
    if (fd < 0) return VFS_ERR;

    fgpt_header_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic = snap->magic;
    strncpy(hdr.name, snap->name, TASK_NAME_LEN);
    hdr.saved_tick = snap->saved_tick;
    hdr.is_user = snap->is_user;
    hdr.cs_saved = snap->cs_saved;
    hdr.ss_saved = snap->ss_saved;
    hdr.regs = snap->regs;
    hdr.stack_base_orig = snap->stack_base_orig;
    hdr.context_esp_offset = snap->context_esp_offset;
    hdr.stack_used = snap->stack_used;
    memcpy(hdr.fd_flags, snap->fd_flags, sizeof(hdr.fd_flags));
    memcpy(hdr.fd_offset, snap->fd_offset, sizeof(hdr.fd_offset));
    memcpy(hdr.fd_present, snap->fd_present, sizeof(hdr.fd_present));
    memcpy(hdr.events, snap->events, sizeof(hdr.events));
    hdr.event_count = snap->event_count;
    hdr.start_time = snap->start_time;
    hdr.user_time = snap->user_time;
    hdr.kernel_time = snap->kernel_time;
    hdr.page_count = snap->page_count;
    memcpy(hdr.page_vaddr, snap->page_vaddr, sizeof(hdr.page_vaddr));
    memcpy(hdr.page_flags, snap->page_flags, sizeof(hdr.page_flags));

    int ok = 1;
    if (sys_write(fd, (uint8_t *)&hdr, sizeof(hdr)) != (int)sizeof(hdr)) ok = 0;
    if (ok && sys_write(fd, snap->stack_data, STACK_PAGE_SIZE) != STACK_PAGE_SIZE) ok = 0;

    for (uint32_t i = 0; ok && i < snap->page_count; i++)
        if (sys_write(fd, snap->page_data[i], 4096) != 4096) ok = 0;

    sys_close(fd);
    return ok ? VFS_OK : VFS_ERR;
}

static void worker_thread(void)
{
    while (1)
    {
        while (work_snapshot == NULL)
        {
            task_remove_ready(worker_task);
            schedule();
        }

        fgpt_snapshot_t *snap = work_snapshot;
        work_snapshot = NULL;

        if (waiter_task)
        {
            task_t *src = waiter_task;
            snap->stack_base_orig = src->kernel_stack_base;
            snap->context_esp_offset = src->context_esp - src->kernel_stack_base;
            snap->stack_used = STACK_PAGE_SIZE;
            memcpy(snap->stack_data, (void *)src->kernel_stack_base, STACK_PAGE_SIZE);
            snap->regs = src->regs;
        }

        write_snapshot_to_disk(snap);
        kfree(snap->page_data);
        kfree(snap);

        if (waiter_task)
        {
            task_t *w = waiter_task;
            waiter_task = NULL;
            task_add_ready(w);
        }
    }
}

static void capture_self_snapshot(void)
{
    task_t *self = current_task;

    fgpt_snapshot_t *snap = kmalloc(sizeof(fgpt_snapshot_t));
    if (!snap)
    {
        kprintf("checkpoint: out of memory, save aborted\n");
        return;
    }
    memset(snap, 0, sizeof(fgpt_snapshot_t));

    snap->magic = AEVROSPOINT_MAGIC;
    strncpy(snap->name, self->name, TASK_NAME_LEN);
    snap->saved_tick = get_ticks();
    snap->is_user = self->is_user ? 1 : 0;
    snap->cs_saved = self->is_user ? 0x1B : 0x08;
    snap->ss_saved = self->is_user ? 0x23 : 0x10;

    for (int i = 0; i < TASK_MAX_FDS; i++)
    {
        if (self->fd_table[i])
        {
            snap->fd_present[i] = 1;
            snap->fd_flags[i] = self->fd_table[i]->flags;
            snap->fd_offset[i] = self->fd_table[i]->offset;
        }
    }
    snap->event_count = self->event_count;
    memcpy(snap->events, self->events, sizeof(self->events));
    snap->start_time = self->start_time;
    snap->user_time = self->user_time;
    snap->kernel_time = self->kernel_time;

    if (self->is_user)
        snapshot_user_pages(self->cr3, snap);

    asm volatile("cli");
    work_snapshot = snap;
    waiter_task = self;
    switch_to_task_state(worker_task, TASK_BLOCKED);
    
    asm volatile("sti");

    (void)snap;
}

static int do_actual_save_to_file(task_t *target)
{
    if (target->kernel_stack_base == 0)
    {
        kprintf("checkpoint: task '%s' has no saveable kernel stack\n", target->name);
        return VFS_ERR;
    }

    asm volatile("cli");
    task_state_t orig_state = target->state;
    if (orig_state == TASK_READY)
        target->state = TASK_BLOCKED;
    asm volatile("sti");

    fgpt_snapshot_t *snap = kmalloc(sizeof(fgpt_snapshot_t));
    if (!snap)
    {
        if (orig_state == TASK_READY) target->state = TASK_READY;
        return VFS_ERR;
    }
    memset(snap, 0, sizeof(fgpt_snapshot_t));

    snap->magic = AEVROSPOINT_MAGIC;
    strncpy(snap->name, target->name, TASK_NAME_LEN);
    snap->saved_tick = get_ticks();
    snap->is_user = target->is_user ? 1 : 0;
    snap->cs_saved = target->is_user ? 0x1B : 0x08;
    snap->ss_saved = target->is_user ? 0x23 : 0x10;
    snap->regs = target->regs;
    snap->stack_base_orig = target->kernel_stack_base;
    snap->context_esp_offset = target->context_esp - target->kernel_stack_base;
    snap->stack_used = STACK_PAGE_SIZE;
    memcpy(snap->stack_data, (void *)target->kernel_stack_base, STACK_PAGE_SIZE);

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

    if (target->is_user)
        snapshot_user_pages(target->cr3, snap);

    if (orig_state == TASK_READY)
        target->state = TASK_READY;

    int result = write_snapshot_to_disk(snap);
    kfree(snap->page_data);
    kfree(snap);
    return result;
}

int aevrospoint_save(const char *name)
{
    if (!name) return VFS_ERR;
    task_t *target = find_task_by_name(name);
    if (!target)
    {
        kprintf("checkpoint: task '%s' not found\n", name);
        return VFS_ERR;
    }

    if (target == current_task)
    {
        capture_self_snapshot();
        
        while (waiter_task == current_task || work_snapshot != NULL)
            schedule();
        return VFS_OK;
    }

    return do_actual_save_to_file(target);
}

int aevrospoint_restore(const char *name)
{
    if (!name) return VFS_ERR;
    char path[64];
    build_path(name, path, sizeof(path));
    int fd = sys_open(path, READ_ONLY);
    if (fd < 0)
    {
        kprintf("checkpoint: no snapshot for '%s'\n", name);
        return VFS_ERR;
    }
    fgpt_header_t *hdr = kmalloc(sizeof(fgpt_header_t));
    if (!hdr) { sys_close(fd); return VFS_ERR; }
    int hdr_read = sys_read(fd, (uint8_t *)hdr, sizeof(fgpt_header_t));
    if (hdr_read != (int)sizeof(fgpt_header_t) || hdr->magic != AEVROSPOINT_MAGIC)
    {
        sys_close(fd);
        kfree(hdr);
        kprintf("checkpoint: '%s' is not a valid checkpoint file\n", name);
        return VFS_ERR;
    }
    fgpt_snapshot_t *snap = kmalloc(sizeof(fgpt_snapshot_t));
    if (!snap) { sys_close(fd); kfree(hdr); return VFS_ERR; }
    memset(snap, 0, sizeof(fgpt_snapshot_t));
    snap->magic = hdr->magic;
    strncpy(snap->name, hdr->name, TASK_NAME_LEN);
    snap->saved_tick = hdr->saved_tick;
    snap->is_user = hdr->is_user;
    snap->cs_saved = hdr->cs_saved;
    snap->ss_saved = hdr->ss_saved;
    snap->regs = hdr->regs;
    snap->stack_base_orig = hdr->stack_base_orig;
    snap->context_esp_offset = hdr->context_esp_offset;
    snap->stack_used = hdr->stack_used;
    memcpy(snap->fd_flags, hdr->fd_flags, sizeof(hdr->fd_flags));
    memcpy(snap->fd_offset, hdr->fd_offset, sizeof(hdr->fd_offset));
    memcpy(snap->fd_present, hdr->fd_present, sizeof(hdr->fd_present));
    memcpy(snap->events, hdr->events, sizeof(hdr->events));
    snap->event_count = hdr->event_count;
    snap->start_time = hdr->start_time;
    snap->user_time = hdr->user_time;
    snap->kernel_time = hdr->kernel_time;
    snap->page_count = hdr->page_count;
    memcpy(snap->page_vaddr, hdr->page_vaddr, sizeof(hdr->page_vaddr));
    memcpy(snap->page_flags, hdr->page_flags, sizeof(hdr->page_flags));
    kfree(hdr);

    int stack_read = sys_read(fd, snap->stack_data, STACK_PAGE_SIZE);
    if (stack_read != (int)STACK_PAGE_SIZE) { sys_close(fd); kfree(snap); return VFS_ERR; }

    if (snap->page_count > 0)
    {
        if (snap->page_count > FP_MAX_PAGES)
        {
            sys_close(fd);
            kfree(snap);
            kprintf("checkpoint: '%s' has an invalid page_count\n", name);
            return VFS_ERR;
        }
        snap->page_data = kmalloc(snap->page_count * 4096);
        if (!snap->page_data)
        {
            sys_close(fd);
            kfree(snap);
            return VFS_ERR;
        }
        uint32_t page_bytes = snap->page_count * 4096;
        uint32_t got_total = 0;
        uint8_t *pptr = (uint8_t *)snap->page_data;
        while (got_total < page_bytes)
        {
            uint32_t remaining = page_bytes - got_total;
            uint32_t chunk = (remaining < 4096) ? remaining : 4096;
            int got = sys_read(fd, pptr + got_total, chunk);
            if (got <= 0) break;
            got_total += got;
        }
        if (got_total != page_bytes) { sys_close(fd); kfree(snap->page_data); kfree(snap); return VFS_ERR; }
    }
    sys_close(fd);

    task_t *task = kmalloc(sizeof(task_t));
    if (!task) { kfree(snap->page_data); kfree(snap); return VFS_ERR; }
    memset(task, 0, sizeof(task_t));
    uint8_t *stack_base = kmalloc(STACK_PAGE_SIZE);
    if (!stack_base) { kfree(task); kfree(snap->page_data); kfree(snap); return VFS_ERR; }

    task->pid = next_pid++;
    task->state = TASK_READY;
    task->parent = current_task;
    strncpy(task->name, snap->name, TASK_NAME_LEN);
    task->is_user = snap->is_user;

    if (snap->is_user)
    {
        uint32_t new_cr3 = restore_user_pages(snap);
        if (!new_cr3) { kfree(stack_base); kfree(task); kfree(snap->page_data); kfree(snap); return VFS_ERR; }
        task->cr3 = new_cr3;
    }
    else
    {
        asm volatile("mov %%cr3, %0" : "=r"(task->cr3));
    }

    memcpy(stack_base, snap->stack_data, STACK_PAGE_SIZE);
    relocate_stack_pointers(stack_base, snap->stack_base_orig, (uint32_t)stack_base);

    uint32_t stack_top = (uint32_t)stack_base + STACK_PAGE_SIZE;
    task->kernel_stack = stack_top;
    task->kernel_stack_base = (uint32_t)stack_base;
    task->context_esp = (uint32_t)stack_base + snap->context_esp_offset;
    task->regs = snap->regs;
    task->regs.cs = snap->cs_saved;
    task->regs.ss = snap->ss_saved;
    task->first_run = 0;
    task->is_checkpoint_clone = 1;

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
    kfree(snap->page_data);
    kfree(snap);
    task_register_all(task);
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
    if (!found) kprintf("    (none saved yet)\n");
    sys_close(fd);
    kprintf("\n");
}

void aevrospoint_init(void)
{
    uint8_t *ws = kmalloc(4096);
    if (!ws) return;
    worker_task = kmalloc(sizeof(task_t));
    if (!worker_task) { kfree(ws); return; }
    memset(worker_task, 0, sizeof(task_t));
    worker_task->pid = next_pid++;
    
    strncpy(worker_task->name, "ckptworker", TASK_NAME_LEN);
    worker_task->state = TASK_READY;
    worker_task->is_user = 0;

    worker_task->kernel_stack = (uint32_t)ws + 4096;
    worker_task->kernel_stack_base = (uint32_t)ws;
    worker_task->context_esp = build_initial_stack(ws, (uint32_t)worker_thread, 0x08, 0x10, worker_task->kernel_stack);
    asm volatile("mov %%cr3, %0" : "=r"(worker_task->cr3));
    worker_task->first_run = 1;

    task_register_all(worker_task);
    task_remove_ready(worker_task);
}
