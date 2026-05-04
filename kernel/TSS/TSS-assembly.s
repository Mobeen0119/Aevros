global jump_user_mode
extern user_stack_top
extern user_function

jump_user_mode:

cli
mov ax,0x23
mov ds,ax
mov fs,ax
mov es,ax
mov gs,ax

push 0x23
push user_stack_top

pushf
or dword[esp],0x200

push 0x1B
push user_function

iret