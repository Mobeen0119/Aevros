global syscall_entry

extern syscall_handler

syscall_entry:
    
    pusha

    push ds
    push es 
    push fs
    push gs

    mov ax,0x10 

    mov ds,ax
    mov es,ax

    push esp

    call syscall_handler

    add esp,4

    pop gs
    pop fs
    pop es
    pop ds

    popa 

    iretd
