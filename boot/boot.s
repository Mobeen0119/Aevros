extern kernel_main

section .multiboot
align 4
    dd 0x1BADB002            ; Multiboot magic number
    dd 0x0                   ; Flags
    dd -(0x1BADB002)         ; Checksum

section .text
global _start

_start:
    cli                      ; Clear interrupts
   
    mov byte [0xB8000], 'H'
    mov byte [0xB8001], 0x0F
    mov byte [0xB8002], 'I'
    mov byte [0xB8003], 0x0F
    
    ; Force flat segment registers to ensure we are pointing to the correct base addresses
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov esp, stack_top       ; Set up the stack pointer

    ; --- VISUAL TEST: FLOOD SCREEN WITH BLUE '!' CHARACTERS ---
    mov edi, 0xB8000
    mov ecx, 80 * 25
.loop:
    mov word [edi], 0x1F21   ; 0x1F = Blue background/White text, 0x21 = '!'
    add edi, 2
    loop .loop
    ; ----------------------------------------------------------

    call kernel_main         ; Jump to your C code execution context

.hang:
    cli
    hlt
    jmp .hang

section .bss
align 16
stack_bottom:
    resb 16384               ; Allocate 16KB stack space
stack_top: