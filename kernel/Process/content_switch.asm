global switch_current_task
global read_eip

extern current_task
extern tss

CR3_OFFSET          equ 0
ESP_OFFSET          equ 28
EBP_OFFSET          equ 24
KERNEL_STACK_OFFSET equ 72
FIRST_RUN_OFFSET    equ 228

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

    mov [eax + ESP_OFFSET], esp

.load_next:
    mov [current_task], ecx

    mov edx, [ecx + CR3_OFFSET]
    mov cr3, edx

    mov edx, [ecx + KERNEL_STACK_OFFSET]
    mov [tss+4], edx

    mov esp, [ecx + ESP_OFFSET]

    cmp dword [ecx + FIRST_RUN_OFFSET], 0
    je .resume_normal

    mov dword [ecx + FIRST_RUN_OFFSET], 0
    mov al, 'F'
    out 0xE9, al
    pop edi
    pop esi
    pop ebx
    pop ebp
    iret

.resume_normal:
    pop edi
    pop esi
    pop ebx
    pop ebp
    popf
    pushf
    and dword [esp], 0xFFFFFEFF
    popf
    ret

read_eip:
    mov eax, [esp]
    ret