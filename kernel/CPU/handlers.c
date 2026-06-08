#include "../../Lib/kprintf.h" 

struct registers {
    unsigned int gs, fs, es, ds;
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;
    unsigned int int_no, err_code;
};


void irq_handler(struct registers *r) {
    if (r->int_no >= 40) {
    }

    if (r->int_no == 32) {
        kprint("Timer IRQ received!\n");
    } else if (r->int_no == 33) {
        kprint("Keyboard IRQ received!\n");
    }
}