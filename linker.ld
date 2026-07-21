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

    .aevros_directory : {
        __directory_start = .;
        KEEP(*(.aevros_directory))
        __directory_end = .;
    }

     .aevros_ip_directory : {
        __ip_directory_start = .;
        KEEP(*(.aevros_ip_directory))
        __ip_directory_end = .;
    }
    
    .data : {
        *(.data*)
    }

    .bss : {
        *(COMMON)
        *(.bss*)
    }  
    . = ALIGN(4096);
    kernel_end = .;
    
/DISCARD/ : {
    *(.eh_frame*)
    *(.got*)
    *(.got.plt*)
    *(.note.gnu.property)
    *(.note*)
    *(.comment)
}
}