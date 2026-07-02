[bits 32]

global context_switch
global trap_return
global read_eip

extern current_task
extern tss

CR3_OFFSET     equ 0
REGS_OFFSET    equ 12
KSTACK_OFFSET  equ 76
IS_USER_OFFSET equ 628

context_switch:
    mov eax, [esp+4]
    mov edx, [esp+8]

    push ebp
    push ebx
    push esi
    push edi

    mov [eax], esp

    mov esp, edx

    pop edi
    pop esi
    pop ebx
    pop ebp

    ret

trap_return:
    mov ecx, [current_task]

    mov edx, [ecx + CR3_OFFSET]
    mov cr3, edx

    mov edx, [ecx + KSTACK_OFFSET]
    mov [tss+4], edx

    cmp byte [ecx + IS_USER_OFFSET], 0
    je .kernel_segs

    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    jmp .frame

.kernel_segs:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

.frame:
    pop eax
    mov ds, ax
    popa
    add esp, 8
    iret

read_eip:
    mov eax, [esp]
    ret

