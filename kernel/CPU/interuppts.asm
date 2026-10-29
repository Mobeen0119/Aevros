[bits 32]

global isr13
extern irq_handler
extern isr_handler

global irq0_handler
global irq1_handler
global irq12_handler
global irq2_handler
global irq9_handler
global irq10_handler
global irq11_handler
global irq7_handler
global irq15_handler
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
IRQ 2, 34
IRQ 1, 33
IRQ 12, 44
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 7, 39
IRQ 15, 47

extern unhandled_vector

global dflt_table

%macro DFLT 1
dflt%1:
    push dword %1
    jmp dflt_common
%endmacro

%assign dflt_i 0
%rep 256
DFLT dflt_i
%assign dflt_i dflt_i+1
%endrep

dflt_common:
    call unhandled_vector
.hang:
    hlt
    jmp .hang

section .data
dflt_table:
%assign dflt_i 0
%rep 256
    dd dflt %+ dflt_i
%assign dflt_i dflt_i+1
%endrep