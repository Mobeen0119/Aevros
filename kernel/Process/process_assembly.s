global switch_current_task
switch_current_task:
    push eax
    push ebx
    push ecx
    push edx
    push esi
    push edi
    push ebp

    extern current_task
    extern tss_entry
    mov eax, [current_task]
    mov [eax+0], esp

    mov ecx, [esp+28]
    mov [eax +8 ], ecx ; Update global current_task

    mov eax, [ecx+12]
    mov cr3, eax

    mov esp, [ecx+4]
    mov ebp, [ecx+8]

    mov[tss_entry + 4], eax ; Update TSS esp0

    pop eax
    pop ebx
    pop ecx
    pop edx
    pop esi
    pop edi
    pop ebp
    
    jmp [eax +8]

