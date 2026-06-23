global syscall_asm_handler
extern syscall_handler

syscall_asm_handler:
    push 0
    push 0x80

    pusha

    xor eax,eax 
    mov ax, ds 
    push eax

    mov ax,0x10 ;kernel data segment selector
    mov ds,ax
    mov es,ax
    mov fs,ax
    mov gs,ax

    push esp ; ptr to Saved register
    call syscall_handler
    add esp, 4

    pop eax 
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa
    add esp, 8 
    
    iretd