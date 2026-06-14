global timer_handler_interrupt

extern schedule

timer_handler_interrupt:
    pusha

    mov al, 0x20 ; Send End of Interrupt signal to the PIC
    out 0x20, al

    
    popa 
    iret