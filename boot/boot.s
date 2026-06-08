extern kernel_main

section .text
global _start

_start:
    jmp _start_real

align 4
multiboot_header:
    dd 0x1BADB002
    dd 0x00
    dd -(0x1BADB002 + 0x00)

_start_real:
    cli

    mov byte [0xB8000], 'H'
    mov byte [0xB8001], 0x0F
    mov byte [0xB8002], 'I'
    mov byte [0xB8003], 0x0F

    mov esp, stack_top

    mov byte [0xB8004], '2'
    mov byte [0xB8005], 0x0F

    call kernel_main

.hang:
    cli
    hlt
    jmp .hang

section .bss
align 16
stack_bottom:
    resb 16384
stack_top: