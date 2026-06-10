global switch_current_task
global read_eip

extern current_task
extern tss

CR3_OFFSET          equ 0
ESP_OFFSET          equ 28
EBP_OFFSET          equ 24
KERNEL_STACK_OFFSET equ 68

switch_current_task:
    mov eax, [esp+4]    
    mov ecx, [esp+8]    

    test eax, eax
    jz .load_next

    mov [eax + ESP_OFFSET], esp
    mov [eax + EBP_OFFSET], ebp

.load_next:
    mov [current_task], ecx

    mov edx, [ecx + CR3_OFFSET]
    mov cr3, edx

    mov edx, [ecx + KERNEL_STACK_OFFSET]
    mov [tss+4], edx


    iret  
      
read_eip:
    mov eax, [esp]
    ret