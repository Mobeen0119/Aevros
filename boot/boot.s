extern kernel_main

section .multiboot
align 4
	dd      0x1BADB002							; Magic number
	dd      0								; Flags
	dd      -(0x1BADB002 )				; Checksum

section .text
global _start

_start:
	cli
	mov esp,stack_top
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


