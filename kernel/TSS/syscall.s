extern syscall_handler

[GLOBAL syscall_asm_handler]

syscall_asm_handler:
    cli
    push 0
    push 0x80

    pusha

    mov ax, ds 
    push eax ; Push the segment selector for the user data segment

    mov ax,0x10 ;kernel data segment selector
    mov ds,ax
    mov es,ax
    mov fs,ax
    mov gs,ax

    push esp ; Push the user stack pointer
    call syscall_handler
    add esp, 4 ; Clean up stack after the call

    pop eax ; Pop the segment selector for the user data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa
    add esp, 8 ; Clean up the two values we pushed at the start
    iret