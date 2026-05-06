[GLOBAL jump_user_mode]
jump_user_mode:

    cli

    mov eax,[esp+4] ; Load the address of the user function into eax
    mov ebx,[esp+8] ; Load the user stack top into ebx

    mov ax,0x23
    mov ds,ax
    mov fs,ax
    mov es,ax
    mov gs,ax

    push 0x23  ; data segment for user
    push ebx ; Push the user stack top onto the stack

    push 0x200 ; EFLAGS with interrupts enabled

    push 0x1B  ; code segment for user
    push eax

    iret