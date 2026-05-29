global enter_usermode

enter_usermode:

    cli
    mov ax,0x23  
    mov ds,ax
    mov es,ax
    mov fs,ax
    mov gs,ax


    push 0x23 

    push ebx 

    pushf

    pop eax 
    or eax,0x200 
    push eax

    push 0x1B 

    push ecx 

    iretd
