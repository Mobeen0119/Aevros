extern _main

section .multiboot 
align 4
dd 0x1BADB002 ; Magic number
dd 0x00 ; Flags
dd -(0x1BADB002 + 0x00) ; Checksum

section .setup_data
align 4096
boot_page_directory:
dd 0x00000083 ;Present, Read/Write, Supervisor

time 767 dd 0 ; 767 entries for the page directory (4KB pages)
dd 0x00000083

time 254 dd 0 ; 254 entries for the page directory (4KB pages)
dd (boot_page_directory_phy_addr) | 0x003 ; Last entry points to the page directory itself

section .text
_start:
mov eax,boot_page_directory_phy_addr
mov cr3,eax ; Load the page directory address into CR3

mov eax,cr4
or eax , 0x00000010 ; Enable PAE
mov cr3,eax ; Load the page directory address into CR3 again to apply PAE

lea eax, [higher_half]
jmp eax
 
 higher_half:
 mov dword[boot_page_directory], 0
 invlpg [0] 
 call kernel_main



[bits 32]
global _start       ;
extern _main

_start:
cli ; Clear interrupts
mov esp,stack_space
call  _main 
hlt ; Halt the CPU

section .bss
resb 8192
stack_space:

[GLOBAL idt_load]
idt_load:
mov,edx,[esp+4]
lidt[edx]
ret