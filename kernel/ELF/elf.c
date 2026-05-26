#include "elf.h"

int elf_validate(Elf32_Header *hdr){

    if(!hdr) return 0;

    uint32_t *magic=(uint32_t*)hdr->identity;

    if(*magic!=ELF_MAGIC_NUMBER) return 0;

    return 1;
}