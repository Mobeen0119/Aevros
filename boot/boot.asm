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
    
    ; Enable When GUI renders
  ;  dd 0x00000004          ; bit 2: request a graphics mode
   ; dd -(0x1BADB002 + 0x00000004)
    ;dd 0                   ; header_addr - unused (bit 16 not set), but GRUB
    ;dd 0                   ; load_addr     always reads this as a fixed-size
    ;dd 0                   ; load_end_addr struct, so these 5 fields have to
    ;dd 0                   ; bss_end_addr  physically exist even though their
    ;dd 0                   ; entry_addr    values are ignored
    ;dd 0                   ; mode_type: 0 = linear graphics (not text)
    ;dd 1024                ; preferred width
    ;dd 768                 ; preferred height
    ;dd 32                  ; preferred bits per pixel

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