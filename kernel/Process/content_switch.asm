global switch_current_task
global read_eip

extern current_task
extern tss

CR3_OFFSET          equ 0
ESP_OFFSET          equ 28
EBP_OFFSET          equ 24
KERNEL_STACK_OFFSET equ 72

switch_current_task:
    mov eax, [esp+4]        ; prev task
    mov ecx, [esp+8]        ; next task

    test eax, eax
    jz .load_next

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

    pop edi
    pop esi
    pop ebx
    pop ebp

    iret

read_eip:
    mov eax, [esp]
    ret