global jump_user_mode

jump_user_mode:

    cli

    mov eax,[esp+4]
    mov ebx,[esp+8]

    mov ax,0x23
    mov ds,ax
    mov fs,ax
    mov es,ax
    mov gs,ax

    push 0x23
    push ebx

    pushfd 
    pop ecx
    or ecx,0x200
    push ecx
   

    push 0x1B
    push eax

    iretd 