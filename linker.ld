ENTRY(_start)

SECTIONS
{
    /* The kernel starts loading at 1MB physical address space */
    . = 0x00100000;

    .text : {
        *(.multiboot)         /* Forces the multiboot header block to the front */
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

    /* Discard modern compiler note payloads that push the multiboot header out of range */
    /DISCARD/ : {
        *(.note.gnu.property)
        *(.note*)
        *(.comment)
    }
}