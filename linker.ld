ENTRY(_start)

SECTIONS
{
    . = 0xC0100000;  //Virtual address where the kernel will be loaded

    .text : AT(ADDR(.text)-0xC0000000) {  //load at physical 1MB
        *(.multiboot)
        *(.text)
    }

    .rodata : {
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    .bss : AT(ADDR(.bss) - 0xC0000000){
        *(COMMON)
        *(.bss)
    }
}