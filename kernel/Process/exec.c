#include "exec.h"
#include "task.h"

#include "../Paging/paging.h"
#include "../Memory/pmm.h"
#include "../Memory/kheap.h"
#include "../ELF/elf.h"
#include "process-memory/process_memory.h"
#include "../../Lib/string.h"

#include "../../Include/vfs.h"
#include "TaskLife/tasklife.h"

static inline uint32_t read_cr3()
{
    uint32_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

int exec_user(void *binary, uint32_t size)
{
    if (!binary || !size)
        return VFS_ERR;

    uint32_t new_cr3 = create_user_space();

    if (!new_cr3)
        return VFS_ERR;

    Elf32_Header *hdr = (Elf32_Header *)binary;

    if (!elf_validate(hdr))
    {
        destroy_user_space(new_cr3);
        return VFS_ERR;
    }

    if (!elf_load_segs(hdr, new_cr3))
    {
        destroy_user_space(new_cr3);
        return VFS_ERR;
    }

    uint32_t stack_phy = pmm_alloc();

    if (!stack_phy)
    {
        destroy_user_space(new_cr3);
        return VFS_ERR;
    }

    if (!(map_page_in_directory(
            new_cr3,
            USER_STACK_TOP - USER_STACK_SIZE,
            stack_phy,
            PAGE_PRESENT | PAGE_USER | PAGE_WRITE)))
    {
        pmm_free(stack_phy);
        destroy_user_space(new_cr3);
        return VFS_ERR;
    }

    task_t *task = kmalloc(sizeof(task_t));

    if (!task)
    {
        destroy_user_space(new_cr3);
        return VFS_ERR;
    }

    memset(task, 0, sizeof(task_t));

    void *kstack = kmalloc(4096);

    if (!kstack)
    {
        kfree_raw(task);
        destroy_user_space(new_cr3);
        return VFS_ERR;
    }

    task->pid = next_pid++;
    task->state = TASK_READY;
    task->cr3 = new_cr3;
    task->regs.eip = hdr->entry_point;
    task->first_run = 1;
    task->is_user = 1;

    task->regs.ebp = USER_STACK_TOP;
    task->kernel_stack = (uint32_t)kstack + 4096;
    task->kernel_stack_base = (uint32_t)kstack;
    task->cwd = current_task->cwd;
    
    task->parent = current_task;
    task->start_time = get_ticks();

    if (task->cwd)
        task->cwd->ref_count++;

    uint32_t *sp = (uint32_t *)((uint32_t)kstack + 4096);
    *(--sp) = 0x23 | 3;         // SS
    *(--sp) = USER_STACK_TOP;   // ESP
    *(--sp) = 0x202;            // EFLAGS
    *(--sp) = 0x1B | 3;         // CS
    *(--sp) = hdr->entry_point; // EIP

    *(--sp) = 0; // edi
    *(--sp) = 0; // esi
    *(--sp) = 0; // ebx
    *(--sp) = 0; // ebp
    *(--sp) = 0; // eax

    task->regs.esp = (uint32_t)sp;

    for (int i = 0; i < TASK_MAX_FDS; i++)
    {
        if (!current_task->fd_table[i])
            continue;
        if (current_task->fd_table[i]->flags & O_CLOEXEC)
        {
            task->fd_table[i] = NULL;
            continue;
        }
        task->fd_table[i] = current_task->fd_table[i];
        task->fd_table[i]->inode->ref_count++;
    }

    task_add_ready(task);

    return task->pid;
}

int sys_exec(const char *path)
{
    if (!path)
        return VFS_ERR;

    int fd = sys_open(path, READ_ONLY);

    if (fd < 0)
        return VFS_ERR;

    file_t *file = current_task->fd_table[fd];

    if (!file)
    {
        sys_close(fd);
        return VFS_ERR;
    }

    uint32_t size = file->inode->size;

    void *buffer = kmalloc(size);

    if (!buffer)
    {

        sys_close(fd);

        return VFS_ERR;
    }

    if ((uint32_t)sys_read(fd, buffer, size) != size)
    {
        kfree_raw(buffer);
        sys_close(fd);

        return VFS_ERR;
    }

    sys_close(fd);

    Elf32_Header *hdr = (Elf32_Header *)buffer;

    if (!elf_validate(hdr))
    {
        kfree_raw(buffer);
        return VFS_ERR;
    }

    uint32_t new_cr3 = create_user_space();

    if (!new_cr3)
    {
        kfree_raw(buffer);
        return VFS_ERR;
    }

    if (!elf_load_segs(hdr, new_cr3))
    {
        destroy_user_space(new_cr3);
        kfree_raw(buffer);

        return VFS_ERR;
    }

    uint32_t stack_phy = pmm_alloc();

    if (!stack_phy)
    {
        destroy_user_space(new_cr3);
        kfree_raw(buffer);
        return VFS_ERR;
    }

    if (!(map_page_in_directory(new_cr3, USER_STACK_TOP - USER_STACK_SIZE,
                                stack_phy, PAGE_PRESENT | PAGE_USER | PAGE_WRITE)))
    {
        pmm_free(stack_phy);
        destroy_user_space(new_cr3);
        kfree_raw(buffer);
        return VFS_ERR;
    }

    for (int i = 0; i < TASK_MAX_FDS; i++)
    {
        if (current_task->fd_table[i] &&
            current_task->fd_table[i]->flags & O_CLOEXEC)
            sys_close(i);
    }

    uint32_t old_cr3 = current_task->cr3;
    current_task->cr3 = new_cr3;
    task_log_event(current_task, EVT_EXEC, 0);

    current_task->regs.eip = hdr->entry_point;
    current_task->regs.ebp = USER_STACK_TOP;

    current_task->signal_mask=0;
    current_task->pending_signals=0;

    for(int i=0;i<NSIGNALS;i++) current_task->signal_handlers[i]=NULL;

    current_task->user_time=0;
    current_task->kernel_time=0;
    current_task->start_time=get_ticks();
    current_task->first_run = 1;
    current_task->is_user = 1;

    strncpy(current_task->name, path, TASK_NAME_LEN);

    uint32_t *sp = (uint32_t *)current_task->kernel_stack;
    *(--sp) = 0x23 | 3;         // SS
    *(--sp) = USER_STACK_TOP;   // ESP
    *(--sp) = 0x202;            // EFLAGS=
    *(--sp) = 0x1B | 3;         // CS
    *(--sp) = hdr->entry_point; // EIP

    // Saved registers
    *(--sp) = 0; // edi
    *(--sp) = 0; // esi
    *(--sp) = 0; // ebx
    *(--sp) = 0; // ebp
    *(--sp) = 0; // eax

    current_task->regs.esp = (uint32_t)sp; // Point to iret frame on kernel stack

    destroy_user_space(old_cr3);

    kfree_raw(buffer);

    return 0;
}
