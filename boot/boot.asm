extern kernel_main

section .text
global _start

_start:
    jmp _start_real

align 8
multiboot_header:
    dd 0xE85250D6                                        ; multiboot2 magic
    dd 0                                                  ; architecture: 0 = i386 protected mode
    dd multiboot_header_end - multiboot_header            ; header length
    dd -(0xE85250D6 + 0 + (multiboot_header_end - multiboot_header))  ; checksum

    align 8
    dw 5                    ; tag type 5: framebuffer
    dw 0                    ; flags
    dd 20                   ; tag size: 8 header bytes + width/height/depth (4 each)
    dd 1024                 ; width
    dd 768                  ; height
    dd 32                   ; preferred bits per pixel

    align 8
    dw 0                    ; tag type 0: end of tags
    dw 0
    dd 8
multiboot_header_end:

_start_real:
    cli

    mov byte [0xB8000], 'H'
    mov byte [0xB8001], 0x0F
    mov byte [0xB8002], 'I'
    mov byte [0xB8003], 0x0F

    mov esp, stack_top

    mov byte [0xB8004], '2'
    mov byte [0xB8005], 0x0F

    
    push ebx
    push eax
    call kernel_main
    add esp, 8

.hang:
    cli
    hlt
    jmp .hang

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:
