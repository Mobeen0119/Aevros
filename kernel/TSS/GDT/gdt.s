[GLOBAL gdt_flush]

gdt_flush:
    cli
    mov eax, [esp + 4] ; Load the address of the GDT into eax
    lgdt [eax]

    jmp 0x08:flush ; Jump to the code segment selector (0x08) and flush the instruction pipeline

    flush:
    mov ax,0x10 
    mov ds,ax
    mov es,ax
    mov fs,ax
    mov gs,ax
    mov ss,ax

    sti 
    ret