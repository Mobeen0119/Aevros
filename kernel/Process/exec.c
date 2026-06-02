#include "exec.h"
                                                              
#include "task.h"
#include "../Paging/paging.h"
#include "../Memory/pmm.h"
#include "../Memory/kheap.h"
#include "../ELF/elf.h"

#include "../../Include/vfs.h"

int exec_user(void *binary, uint32_t size)
{
    if (!binary || !size)
        return VFS_ERR;

    uint32_t new_cr3 = clone_page_directory(read_cr3());

    Elf32_Header *hdr = (Elf32_Header *)binary;

    if (!elf_validate(hdr))
        return VFS_ERR;

    if (!elf_load_segs(hdr, new_cr3))
        return VFS_ERR;

    uint32_t stack_phy = pmm_alloc();

    map_page_in_directory(new_cr3, USER_STACK_TOP - USER_STACK_SIZE, stack_phy, PAGE_PRESENT | PAGE_USER | PAGE_WRITE);

    task_t *task = kmalloc(sizeof(task_t));

    if (!task)
        return VFS_ERR;

    memset(task, 0, sizeof(task_t));

    task->pid = next_pid++;
    task->state = TASK_READY;
    task->cr3 = new_cr3;
    task->regs.eip = hdr->entry_point;

    task->regs.esp = USER_STACK_TOP;
    task->regs.ebp = USER_STACK_TOP;

    void *kstack = kmalloc(4096);
    if (!kstack)
        return VFS_ERR;

    task->kernel_stack = (uint32_t)kstack + (4096);

    for (int i = 0; i < 32; i++)
    {
        task->fd_table[i] = current_task->fd_table[i];

        if (task->fd_table[i])
            task->fd_table[i]->inode->ref_count++;
    }

    if (!ready_queue)
    {
        ready_queue = task;
        task->next = task;
    }
    else
    {
        task_t *temp = ready_queue;

        while (temp->next != ready_queue)
            temp = temp->next;
        temp->next = task;
        task->next = ready_queue;
    }

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
        return VFS_ERR;

    uint32_t size = file->inode->size;

    void *buffer = kmalloc(size);

    if (!buffer)
    {
        kfree(buffer);
        sys_close(fd);

        return VFS_ERR;
    }

    if (sys_read(fd, buffer, size) != size)
    {
        kfree(buffer);
        sys_close(fd);

        return VFS_ERR;
    }

    sys_close(fd);

    Elf32_Header *hdr = (Elf32_Header *)buffer;

    if (!elf_validate(hdr))
    {
        kfree(buffer);
        return VFS_ERR;
    }

    destory_user_space(current_task->cr3);

    if (!elf_load_segs(hdr, current_task->cr3))
    {
        kfree(buffer);

        return VFS_ERR;
    }

    uint32_t stack_phy = pmm_alloc();

    if (!stack_phy)
    {
        kfree(buffer);
        return VFS_ERR;
    }

    map_page_in_directory(current_task->cr3, USER_STACK_TOP - USER_STACK_SIZE,
                          stack_phy, PAGE_PRESENT | PAGE_USER | PAGE_WRITE);

    current_task->regs.eip = hdr->entry_point;
    current_task->regs.useresp = USER_STACK_TOP;

    kfree(buffer);

    return 0;
}
