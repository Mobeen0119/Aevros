ENTRY(_start)

SECTIONS
{

    . = 0x00100000;

    .text : {
        *(.multiboot)        
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
        *(.note.gnu.property)
        *(.note*)
        *(.comment)
    }
}