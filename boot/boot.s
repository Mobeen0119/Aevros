section .multiboot 
align 4
dd 0x1BADB002 ; Magic number
dd 0x00 ; Flags
dd -(0x1BADB002 + 0x00) ; Checksum

_start:
cli ; Clear interrupts
mov esp,stack_space
call main 
hlt ; Halt the CPU

section .bss
resb 8192
stack_space:
