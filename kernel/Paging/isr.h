
#ifndef ISR_H
#define ISR_H

#include <stdint.h>


typedef struct registers {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no;
    uint32_t err_code;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t useresp;   
    uint32_t ss;
} register_t;

#endif 
