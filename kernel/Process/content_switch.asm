global switch_current_task
global read_eip

extern current_task
extern tss

; task_t offsets - must match your struct exactly
CR3_OFFSET          equ 0
ESP_OFFSET          equ 40
EBP_OFFSET          equ 36
KERNEL_STACK_OFFSET equ 68

switch_current_task:
    mov eax, [esp+4]    
    mov ecx, [esp+8]    

    ; save prev task state
    test eax, eax
    jz .load_next

    mov [eax + ESP_OFFSET], esp
    mov [eax + EBP_OFFSET], ebp

.load_next:
    ; update current_task pointer
    mov [current_task], ecx

    ; switch page directory
    mov edx, [ecx + CR3_OFFSET]
    mov cr3, edx

    ; update TSS kernel stack
    mov edx, [ecx + KERNEL_STACK_OFFSET]
    mov [tss+4], edx

    ; restore next task registers
    mov esp, [ecx + ESP_OFFSET]
    mov ebp, [ecx + EBP_OFFSET]

    ret    
read_eip:
    mov eax, [esp]
    ret