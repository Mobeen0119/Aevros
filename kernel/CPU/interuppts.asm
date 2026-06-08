global isr14
global default_handler

default_handler:
    pusha

.loop:
    hlt
    jmp .loop

    
extern isr_handler
isr14:
    cli 

    push dword 14
    pusha
    push ds
    push es
    push fs
    push gs

    mov ax,0x10
    mov ds,ax
    mov es,ax
    mov fs,ax
    mov gs,ax

    push esp

    call isr_handler
    add esp,4 

    pop gs
    pop fs  
    pop es
    pop ds
    popa

    add esp,8
    sti
    iret
