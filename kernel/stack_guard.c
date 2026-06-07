#include <stdint.h>
#include "../Include/screen.h"

uintptr_t __stack_chk_guard = 0xDEADC0DE;

__attribute__((noreturn)) void __stack_chk_fail()
{

    kprint("PANIC MODE : STACK SMASHING DETECT! GO LOOK AT IT");
    
    while (1)
    {
        asm volatile("hlt");s
    }
}