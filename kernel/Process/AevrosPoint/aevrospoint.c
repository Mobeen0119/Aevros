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

typedef struct {
    uint32_t magic;
    char name[TASK_NAME_LEN];
    uint32_t saved_tick;
    uint8_t is_user;
    uint16_t cs_saved;
    uint16_t ss_saved;
    register_t regs;
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

typedef struct {
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

static void build_path(const char *name, char *out, uint32_t outlen) {
    strncpy(out, "/checkpoint_", outlen);
    uint32_t base_len = strlen(out);
    strncpy(out + base_len, name, outlen - base_len - 5);
    strncpy(out + strlen(out), ".ckpt", outlen - strlen(out));
}

static task_t *find_task_by_name(const char *name) {
    if (!ready_queue) return NULL;
    task_t *t = ready_queue;
    do {
        if (strcmp(t->name, name) == 0) return t;
        t = t->next;
    } while (t != ready_queue);
    return NULL;
}

static int snapshot_user_pages(uint32_t cr3, fgpt_snapshot_t *snap) {
    snap->page_count = 0;
    asm volatile("cli");
    map_page(TEMP_PD_VIRT, cr3, PAGE_PRESENT | PAGE_WRITE);
    uint32_t *tar_pd = (uint32_t *)TEMP_PD_VIRT;
    for (int pd = 10; pd < 768; pd++) {
        if (!(tar_pd[pd] & PAGE_PRESENT)) continue;
        uint32_t pt_phy = tar_pd[pd] & 0xFFFFF000;
        map_page(TEMP_PT_VIRT, pt_phy, PAGE_PRESENT | PAGE_WRITE);
        uint32_t *tar_pt = (uint32_t *)TEMP_PT_VIRT;
        for (int pt = 0; pt < 1024; pt++) {
            if (!(tar_pt[pt] & PAGE_PRESENT)) continue;
            if (snap->page_count >= FP_MAX_PAGES) {
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

static uint32_t restore_user_pages(fgpt_snapshot_t *snap) {
    uint32_t *new_pd = (uint32_t *)alloc_page_aligned();
    if (!new_pd) return 0;
    memset(new_pd, 0, 4096);
    uint32_t *current_pd = (uint32_t *)PAGE_RECURSIVE;
    for (int i = 0; i < 10; i++) new_pd[i] = current_pd[i];
    for (int i = 768; i < 1024; i++) new_pd[i] = current_pd[i];
    uint32_t new_pd_phys = (uint32_t)new_pd;
    new_pd[1023] = new_pd_phys | PAGE_PRESENT | PAGE_WRITE;
    for (uint32_t i = 0; i < snap->page_count; i++) {
        uint32_t phys = pmm_alloc();
        if (!phys) {
            kprintf("checkpoint: out of memory restoring page %u/%u\n", i, snap->page_count);
            return 0;
        }
        map_page(TEMP_DST_PAGE, phys, PAGE_PRESENT | PAGE_WRITE);
        memcpy((void *)TEMP_DST_PAGE, snap->page_data[i], 4096);
        unmap(TEMP_DST_PAGE);
        if (!map_page_in_directory(new_pd_phys, snap->page_vaddr[i], phys, snap->page_flags[i])) {
            kprintf("checkpoint: failed mapping restored page %u\n", i);
            return 0;
        }
    }
    return new_pd_phys;
}

int aevrospoint_save(const char *name) {
    if (!name) return VFS_ERR;
    task_t *target = find_task_by_name(name);
    if (!target) {
        kprintf("checkpoint: task '%s' not found\n", name);
        return VFS_ERR;
    }
    int is_current = (target == current_task);
    if (target->kernel_stack_base == 0) {
        kprintf("checkpoint: task '%s' has no saveable kernel stack\n", name);
        return VFS_ERR;
    }
    fgpt_header_t *header = kmalloc(sizeof(fgpt_header_t));
    if (!header) return VFS_ERR;
    memset(header, 0, sizeof(fgpt_header_t));
    header->magic = AEVROSPOINT_MAGIC;
    strncpy(header->name, target->name, TASK_NAME_LEN);
    header->saved_tick = get_ticks();
    header->is_user = target->is_user ? 1 : 0;
    header->cs_saved = target->is_user ? 0x1B : 0x08;
    header->ss_saved = target->is_user ? 0x23 : 0x10;
    if (is_current) {
        asm volatile("cli");
        uint32_t esp, eip, eflags;
        asm volatile("mov %%esp, %0" : "=r"(esp));
        asm volatile("mov (%%esp), %0" : "=r"(eip));
        asm volatile("pushf; pop %0" : "=r"(eflags));
        uint32_t ebp, eax, ebx, ecx, edx, esi, edi;
        asm volatile("mov %%ebp, %0" : "=r"(ebp));
        asm volatile("mov %%eax, %0" : "=r"(eax));
        asm volatile("mov %%ebx, %0" : "=r"(ebx));
        asm volatile("mov %%ecx, %0" : "=r"(ecx));
        asm volatile("mov %%edx, %0" : "=r"(edx));
        asm volatile("mov %%esi, %0" : "=r"(esi));
        asm volatile("mov %%edi, %0" : "=r"(edi));
        header->regs.eax = eax;
        header->regs.ebx = ebx;
        header->regs.ecx = ecx;
        header->regs.edx = edx;
        header->regs.esi = esi;
        header->regs.edi = edi;
        header->regs.ebp = ebp;
        header->regs.esp = esp;
        header->regs.eip = eip;
        header->regs.eflags = eflags;
        uint32_t stack_base = target->kernel_stack_base;
        uint32_t stack_top = target->kernel_stack;
        uint32_t used = stack_top - stack_base;
        used = (used > 4096) ? 4096 : used;
        header->stack_used = used;
        asm volatile("sti");
    } else {
        header->regs = target->regs;
        header->stack_used = 4096;
    }
    for (int i = 0; i < TASK_MAX_FDS; i++) {
        if (target->fd_table[i]) {
            header->fd_present[i] = 1;
            header->fd_flags[i] = target->fd_table[i]->flags;
            header->fd_offset[i] = target->fd_table[i]->offset;
        }
    }
    header->event_count = target->event_count;
    memcpy(header->events, target->events, sizeof(target->events));
    header->start_time = target->start_time;
    header->user_time = target->user_time;
    header->kernel_time = target->kernel_time;
    fgpt_snapshot_t *temp_snap = NULL;
    header->page_count = 0;
    if (target->is_user) {
        temp_snap = kmalloc(sizeof(fgpt_snapshot_t));
        if (temp_snap) {
            memset(temp_snap, 0, sizeof(fgpt_snapshot_t));
            if (snapshot_user_pages(target->cr3, temp_snap) == 0) {
                header->page_count = temp_snap->page_count;
                memcpy(header->page_vaddr, temp_snap->page_vaddr, sizeof(temp_snap->page_vaddr));
                memcpy(header->page_flags, temp_snap->page_flags, sizeof(temp_snap->page_flags));
            }
        }
    }
    char path[64];
    build_path(name, path, sizeof(path));
    int fd = sys_open(path, READ_WRITE | CREAT);
    if (fd < 0) {
        if (temp_snap)
            kfree(temp_snap);
        kfree(header);
        return VFS_ERR;
    }
    int written = sys_write(fd, (uint8_t *)header, sizeof(fgpt_header_t));
    if (written != sizeof(fgpt_header_t)) {
        sys_close(fd);
        if (temp_snap)
            kfree(temp_snap);
        kfree(header);
        return VFS_ERR;
    }
    if (is_current) {
        uint32_t stack_base = target->kernel_stack_base;
        written = sys_write(fd, (uint8_t *)stack_base, header->stack_used);
    } else {
        written = sys_write(fd, (uint8_t *)target->kernel_stack_base, header->stack_used);
    }
    if (written != header->stack_used) {
        sys_close(fd);
        if (temp_snap)
            kfree(temp_snap);
        kfree(header);
        return VFS_ERR;
    }
    if (header->page_count > 0 && temp_snap) {
        for (uint32_t i = 0; i < temp_snap->page_count && i < header->page_count; i++) {
            written = sys_write(fd, temp_snap->page_data[i], 4096);
            if (written != 4096) {
                sys_close(fd);
                kfree(temp_snap);
                kfree(header);
                return VFS_ERR;
            }
        }
    }
    sys_close(fd);
    if (temp_snap)
        kfree(temp_snap);
    kfree(header);
    return VFS_OK;
}

static int read_exact(int fd, void *buffer, uint32_t size)
{
    uint8_t *ptr = (uint8_t *)buffer;
    uint32_t total = 0;
    while (total < size) {
        int got = sys_read(fd, ptr + total, size - total);
        if (got <= 0)
            return total;
        total += got;
    }
    return total;
}

int aevrospoint_restore(const char *name) {
    if (!name) return VFS_ERR;
    char path[64];
    build_path(name, path, sizeof(path));
    int fd = sys_open(path, READ_ONLY);
    if (fd < 0) {
        kprintf("checkpoint: no snapshot for '%s'\n", name);
        return VFS_ERR;
    }
    fgpt_snapshot_t *snap = kmalloc(sizeof(fgpt_snapshot_t));
    if (!snap) {
        sys_close(fd);
        return VFS_ERR;
    }
    int total_size = sizeof(fgpt_snapshot_t);
    uint8_t *read_ptr = (uint8_t *)snap;
    int bytes_read = 0;
    while (bytes_read < total_size) {
        int remaining = total_size - bytes_read;
        int chunk_size = (remaining < 4096) ? remaining : 4096;
        int got = sys_read(fd, read_ptr + bytes_read, chunk_size);
        if (got <= 0) break;
        bytes_read += got;
    }
    sys_close(fd);
    if (bytes_read != total_size) {
        kfree(snap);
        return VFS_ERR;
    }
    if (snap->magic != AEVROSPOINT_MAGIC) {
        kfree(snap);
        return VFS_ERR;
    }
    task_t *task = kmalloc(sizeof(task_t));
    if (!task) {
        kfree(snap);
        return VFS_ERR;
    }
    memset(task, 0, sizeof(task_t));
    uint8_t *new_stack = kmalloc(4096);
    if (!new_stack) {
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
    task->regs.cs = snap->cs_saved;
    task->regs.ss = snap->ss_saved;
    memcpy(new_stack, snap->stack_data, snap->stack_used);
    task->context_esp = (uint32_t)new_stack + snap->stack_used;
    task->first_run = 0;
    if (snap->is_user) {
        uint32_t new_cr3 = restore_user_pages(snap);
        if (!new_cr3) {
            kfree(new_stack);
            kfree(task);
            kfree(snap);
            return VFS_ERR;
        }
        task->cr3 = new_cr3;
    } else {
        asm volatile("mov %%cr3, %0" : "=r"(task->cr3));
    }
    for (int i = 0; i < TASK_MAX_FDS; i++) {
        if (snap->fd_present[i] && current_task->fd_table[i]) {
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
    task_register_all(task);
    task_add_ready(task);
    return task->pid;
}

void aevrospoint_list(void) {
    int fd = sys_open("/", READ_ONLY);
    if (fd < 0) {
        kprintf("checkpoint: cannot open root\n");
        return;
    }
    dirent_t entry;
    int found = 0;
    set_color(VGA_CYAN, VGA_BLACK);
    kprintf("\n CHECKPOINTS \n");
    kprintf("  -----------------\n");
    reset_color();
    while (sys_readdir(fd, &entry) == 1) {
        if (strncmp(entry.name, "checkpoint_", 11) == 0) {
            kprintf("   %s \n", entry.name);
            found++;
        }
    }
    if (!found) kprintf("    (none saved yet)\n");
    sys_close(fd);
    kprintf("\n");
}


