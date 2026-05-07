global laod_page_directory
load_page_directory:
push ebp
mov ebp,esp
mov eax, [ebp+8] ; get physical address of PD
mov cr3, eax

mov eax, cr0
or eax, 0x80000000 ; pg Bit
mov cr0, eax

mov esp, ebp
    pop ebp
    ret


