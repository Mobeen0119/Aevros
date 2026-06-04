global timer_handler_interrupt

extern schedule

timer_handler_interrupt:
    pusha

    call schedule

    popa 
    iret