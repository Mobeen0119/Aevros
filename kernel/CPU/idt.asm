[BITS 32]
global idt_load

idt_load:
    mov edx, [esp + 4]    ; Get the pointer to the idt_ptr passed from C
    lidt [edx]            ; Load the IDT (Note the space after lidt!)
    ret