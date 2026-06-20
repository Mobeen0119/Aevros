extern kernel_main

section .text
global _start

KERNEL_VIRT_BASE equ 0xC0000000
KERNEL_PAGE_NUM  equ (KERNEL_VIRT_BASE >> 22)   

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

    mov edi, 0x9000          
    mov ecx, 1024
    xor eax, eax

.clear_pd:
    mov [edi], eax
    add edi, 4
    loop .clear_pd

    mov edi, 0xA000          
    mov ecx, 1024
    xor eax, eax
.clear_pt:
    mov [edi], eax
    add edi, 4
    loop .clear_pt

    mov edi, 0xA000
    mov eax, 0x00000003      
    mov ecx, 1024
.fill_pt0:
    mov [edi], eax
    add eax, 0x1000
    add edi, 4
    loop .fill_pt0

    mov dword [0x9000], 0xA003

    mov dword [0x9000 + KERNEL_PAGE_NUM*4], 0xA003

    mov byte [0xB8002], 'P'
    mov byte [0xB8003], 0x0F

    mov eax, 0x9000
    mov cr3, eax

    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    mov byte [0xB8004], 'O'
    mov byte [0xB8005], 0x0F

    lea eax, [higher_half]
    jmp eax

higher_half:
   mov esp, stack_top

    mov byte [0xB8006], 'K'
    mov byte [0xB8007], 0x0F

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