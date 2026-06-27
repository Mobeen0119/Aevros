[bits 32]

global switch_current_task
global read_eip

extern current_task
extern tss

CR3_OFFSET          equ 0
ESP_OFFSET          equ 28
EBP_OFFSET          equ 24
KERNEL_STACK_OFFSET equ 76
FIRST_RUN_OFFSET    equ 232

switch_current_task:

    mov eax, [esp+4]
    mov ecx, [esp+8]

    test eax, eax
    jz .load_next

    pushf
    push ebp
    push ebx
    push esi
    push edi

    mov [eax+ESP_OFFSET], esp
    mov [eax+EBP_OFFSET], ebp

.load_next:

    mov [current_task], ecx

    mov edx, [ecx+CR3_OFFSET]
    mov cr3, edx

    mov edx, [ecx+KERNEL_STACK_OFFSET]
    mov [tss+4], edx

    mov esp, [ecx+ESP_OFFSET]
    mov ebp, [ecx+EBP_OFFSET]

    cmp dword [ecx+FIRST_RUN_OFFSET], 0
    jne .first_run

.resume_normal:

    pop edi
    pop esi
    pop ebx
    pop ebp
    popf
    iret

.first_run:

    mov dword [ecx+FIRST_RUN_OFFSET], 0

    cmp dword [ecx+CR3_OFFSET], 0
    je .kernel

.user:

    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    pop edi
    pop esi
    pop ebx
    pop ebp
    popf
    iret

.kernel:

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    pop edi
    pop esi
    pop ebx
    pop ebp
    iret

read_eip:
    mov eax, [esp]
    ret