global timer_handler_interrupt

extern schedule

timer_handler_interrupt:
    pusha

    call schedule

    popa

    mov al, 0x20
    out 0x20, al

    iret
