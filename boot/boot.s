extern _main


section .multiboot 
align 4
dd 0x1BADB002 ; Magic number
dd 0x00 ; Flags
dd -(0x1BADB002 + 0x00) ; Checksum

[bits 32]
global _start       ; <--- MUST HAVE THIS
extern _main

_start:
cli ; Clear interrupts
mov esp,stack_space
call  _main 
hlt ; Halt the CPU

section .bss
resb 8192
stack_space:
