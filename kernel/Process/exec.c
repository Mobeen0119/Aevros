#include "exec.h"
#include "task.h"

#include "../Paging/paging.h"
#include "../Memory/pmm.h"
#include "../Memory/kheap.h"
#include "../ELF/elf.h"
#include "process-memory/process_memory.h"

#include "../../Include/vfs.h"

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
        return VFS_ERR;

    if (!elf_load_segs(hdr, new_cr3))
        return VFS_ERR;

    uint32_t stack_phy = pmm_alloc();

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
        kfree(task);
        destroy_user_space(new_cr3);
        return VFS_ERR;
    }

    task->pid = next_pid++;
    task->state = TASK_READY;
    task->cr3 = new_cr3;
    task->regs.eip = hdr->entry_point;
    task->regs.ebp = USER_STACK_TOP;


    uint32_t *sp = (uint32_t *)((uint32_t)kstack + 4096);
    *(--sp) = 0x23 | 3;         // SS
    *(--sp) = USER_STACK_TOP;   // ESP 
    *(--sp) = 0x202;            // EFLAGS
    *(--sp) = 0x1B | 3;         // CS 
    *(--sp) = hdr->entry_point; // EIP
    
    
    *(--sp) = 0;  // edi
    *(--sp) = 0;  // esi
    *(--sp) = 0;  // ebx
    *(--sp) = 0;  // ebp

    task->regs.esp = (uint32_t)sp;  
    task->cwd = current_task->cwd;
    task->parent = current_task;

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
        sys_close(fd);
    return VFS_ERR;

    uint32_t size = file->inode->size;

    void *buffer = kmalloc(size);

    if (!buffer)
    {

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

    uint32_t new_cr3 = create_user_space();

    if (!new_cr3)
    {
        kfree(buffer);
        return VFS_ERR;
    }

    if (!elf_load_segs(hdr, new_cr3))
    {
        destroy_user_space(new_cr3);
        kfree(buffer);

        return VFS_ERR;
    }

    uint32_t stack_phy = pmm_alloc();

    if (!stack_phy)
    {
        destroy_user_space(new_cr3);
        kfree(buffer);
        return VFS_ERR;
    }

    if (!(map_page_in_directory(new_cr3, USER_STACK_TOP - USER_STACK_SIZE,
                                stack_phy, PAGE_PRESENT | PAGE_USER | PAGE_WRITE)))
    {
        pmm_free(stack_phy);
        destroy_user_space(new_cr3);
        kfree(buffer);
        return VFS_ERR;
    }

    uint32_t old_cr3 = current_task->cr3;
    current_task->cr3 = new_cr3;

    // Set up iret frame on kernel stack for sys_exec (exec variant of exec_user)
    uint32_t *sp = (uint32_t *)current_task->kernel_stack;
    *(--sp) = 0x23 | 3;         // SS (user data segment with RPL=3)
    *(--sp) = USER_STACK_TOP;   // ESP (user stack top)
    *(--sp) = 0x202;            // EFLAGS (interrupts enabled)
    *(--sp) = 0x1B | 3;         // CS (user code segment with RPL=3)
    *(--sp) = hdr->entry_point; // EIP
    
    // Saved registers
    *(--sp) = 0;  // edi
    *(--sp) = 0;  // esi
    *(--sp) = 0;  // ebx
    *(--sp) = 0;  // ebp

    current_task->regs.eip = hdr->entry_point;
    current_task->regs.esp = (uint32_t)sp;  // Point to iret frame on kernel stack
    current_task->regs.ebp = USER_STACK_TOP;

    destroy_user_space(old_cr3);

    kfree(buffer);

    return 0;
}
