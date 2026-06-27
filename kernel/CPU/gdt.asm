global gdt_flush
global load_tss

gdt_flush:
    cli
    mov eax, [esp + 4]
    lgdt [eax]
    jmp 0x08:flush

flush:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax      
    sti
    ret
	
load_tss:
	mov     ax, 0x28							
	ltr     ax									
	ret