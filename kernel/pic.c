#include <stdint.h>
#include "io.h"
#include "pic.h"

void pic_remap(){
    outb(0x20,0x11);   //Master (0-7)
    outb(0xA0,0x11);  //Slave (8-15)
     
    outb(0x21,0x20);  //Start at 32
    outb(0xA1,0x28);  //Starts at 40
    
    outb(0x21,0x04);
    outb(0xA1,0x02);   //Slave cross Master at 2

    outb(0x21,0x01);
    outb(0xA1,0x01);    //Set to 8086 mode

    outb(0x21,0x0);
    outb(0xA1,0x0);   //Enable Interrupts

    outb(0x21,0xFD);
    outb(0xA1,0xFF);   

}

void pic_eoi(uint8_t irq) {
    if (irq >= 8) outb(0xA0, 0x20);
    outb(0x20, 0x20);
}