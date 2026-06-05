global timer_handler_interrupt

extern schedule

timer_handler_interrupt:
    pusha

    push esp ; Save the current stack pointer for the scheduler
    call schedule
    add esp, 4 ; Clean up the stack after the call to schedule

    mov al, 0x20 ; Send End of Interrupt (EOI) signal to the PIC
    out 0x20, al

    
    popa 
    iret