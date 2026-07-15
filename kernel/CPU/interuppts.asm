[bits 32]

global isr13
extern irq_handler
extern isr_handler

global irq0_handler
global irq1_handler
global isr14
global default_handler

isr13:
    cli
    mov al, 'G'
    out 0xE9, al
    push dword 13
    pusha
    push ds
    push esp
    call isr_handler
    add esp, 4
    pop ds
    popa
    add esp, 4
    sti
    iret

default_handler:
    push 0xDE  
    mov al, 'D'
    out 0xE9, al
    pusha
.loop:
    hlt
    jmp .loop

isr14:
    cli
    push dword 14          
    pusha
    push ds
    push esp
    call isr_handler
    add esp, 4
    pop ds
    popa
    add esp, 8
    sti
    iret

%macro IRQ 2
irq%1_handler:
    cli
    push dword 0
    push dword %2
    pusha
    push ds
    push esp
    call isr_handler
    add esp, 4
    pop ds
    popa
    add esp, 8
    sti
    iret
%endmacro

IRQ 0, 32
IRQ 1, 33