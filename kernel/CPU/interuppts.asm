[bits 32]

; Tell the assembler these symbols are defined elsewhere (in your C files)
extern irq_handler
extern isr_handler

; Expose these to the linker so idt.c can find them
global irq0_handler
global irq1_handler
global isr14
global default_handler

default_handler:
    pusha
.loop:
    hlt
    jmp .loop

isr14:
    cli 
    push dword 14
    pusha
    ; ... (rest of your isr14 code) ...
    call isr_handler
    add esp, 4 
    popa
    sti
    iret

; Macro for IRQ Handlers
%macro IRQ 1
irq%1_handler:
    cli
    pusha
    push ds
    push es
    push fs
    push gs
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    push esp        ; Passing registers struct pointer
    push %1         ; Passing IRQ number
    call irq_handler
    add esp, 8
    pop gs
    pop fs
    pop es
    pop ds
    popa
    sti
    iret
%endmacro

IRQ 0
IRQ 1