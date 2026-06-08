ENTRY(_start)

SECTIONS
{
    . = 0x00100000;

    .text : {
        *(.text*)
    }

    .rodata : {
        *(.rodata*)
    }

    .data : {
        *(.data*)
    }

    .bss : {
        *(COMMON)
        *(.bss*)
    }

/DISCARD/ : {
    *(.eh_frame*)
    *(.got*)    
    *(.got.plt*)       
    *(.got*)            
    *(.note.gnu.property)
    *(.note*)
    *(.comment)
}
}