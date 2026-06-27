global switch_current_task
global read_eip

extern current_task
extern tss

CR3_OFFSET          equ 0
ESP_OFFSET          equ 28
EBP_OFFSET          equ 24
EIP_OFFSET          equ 56
KERNEL_STACK_OFFSET equ 76
FIRST_RUN_OFFSET    equ 232
IS_USER_OFFSET      equ 624

switch_current_task:
    cli

    mov al, '1'
    out 0xE9, al

    mov eax, [esp+4]
    mov ecx, [esp+8]

    test eax, eax
    jz load_next

    mov al, '2'
    out 0xE9, al

    pushf
    push ebp
    push ebx
    push esi
    push edi
    push eax

    mov [eax + ESP_OFFSET], esp
    mov [eax + EBP_OFFSET], ebp

load_next:
    mov al, '3'
    out 0xE9, al

    mov [current_task], ecx

    mov edx, [ecx + CR3_OFFSET]
    mov cr3, edx

    mov al, '4'
    out 0xE9, al

    mov edx, [ecx + KERNEL_STACK_OFFSET]
    mov [tss+4], edx

    mov al, '5'
    out 0xE9, al

     mov esp, [ecx + ESP_OFFSET]
    mov ebp, [ecx + EBP_OFFSET]

    mov al, '6'
    out 0xE9, al

    cmp dword [ecx + FIRST_RUN_OFFSET], 0

    mov al, '9'
    out 0xE9, al

    jne first_run

    mov al, '7'
    out 0xE9, al

    mov al, '7'
    out 0xE9, al

resume_normal:
    pop eax
    pop edi
    pop esi
    pop ebx
    pop ebp
    popf
    mov al, '8'
    out 0xE9, al
    ret

first_run:
    mov dword [ecx + FIRST_RUN_OFFSET], 0x0

    cmp byte [ecx + IS_USER_OFFSET], 0
    je first_run_kernel

first_run_user:
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    jmp first_run_resume

first_run_kernel:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

first_run_resume:
    pop eax
    pop ebp
    pop ebx
    pop esi
    pop edi
    iret
    

read_eip:
    mov eax, [esp]
    ret