#include "elf.h"

int elf_validate(Elf32_Header *hdr){

    if(!hdr) return 0;

    uint32_t *magic=(uint32_t*)hdr->identity;

    if(*magic!=ELF_MAGIC_NUMBER) return 0;

    return 1;
}

Elf32_Header*  elf_prog_header(Elf32_Header* hdr){
    if(!hdr) return NULL;

    return (Elf32_Header*) ((uint8_t*) hdr+hdr->program_header_offset);
}


