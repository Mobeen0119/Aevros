global switch_current_task
extern current 
switch_current_task:
    pusha

    mov eax, [current]
    mov [eax+ESP_OFFSET], esp
    mov [eax+EBP_OFFSET], ebp

    mov ecx, [esp+ARGS_OFFSET] 
    mov [current], ecx 

    mov eax, [ecx+CR3_OFFSET]
    mov cr3, eax

    mov esp, [ecx + ESP_OFFSET]
    mov ebp, [ecx + EBP_OFFSET]

    mov eax, [ecx + KERNEL_STACK]
    mov [tss_entry + 4], eax ; Update TSS esp0

    popa
    
    ret

global read_eip
read_eip:
    mov eax,[esp]
    jmp eax
