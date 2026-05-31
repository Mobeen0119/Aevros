global switch_current_task
extern current_task
extern tss_entry

CR3_OFFSET equ 140
KERNEL STACK equ 144
REGS_BASE equ 164
EBP_OFFSET equ REGS_BASE + 12
ESP_OFFSET equ REGS_BASE + 16


switch_current_task:
    pusha

    mov eax, [current_task]
    test eax,eax ; Check if current_task is NULL
    jz .skip_save ; If it is, skip saving state


    mov [eax+ESP_OFFSET], esp
    mov [eax+EBP_OFFSET], ebp
    call read_e ip

.skip_save:
    mov ecx, [esp+36] 
    mov [current_task], ecx 

    mov edx, [ecx+CR3_OFFSET]
    mov cr3, edx

    mov esp, [ecx + ESP_OFFSET]
    mov ebp, [ecx + EBP_OFFSET]

    mov edx, [ecx + KERNEL_STACK]
    mov [tss_entry + 4], edx ; Update TSS esp0

    popa
    
    ret


global read_eip

read_eip:
    mov eax,[esp]
    jmp eax
