#pragma once

#include <stdint.h>

#define ELF_MAGIC_NUMBER 0x464C457F
#define PT_LOAD 1

typedef struct
{

    uint32_t type, offset, paddr, 
    vaddr, filesz, memsz, flags, align;

} __attribute__((packed)) ELF32_Phdr;

typedef struct {
    uint8_t ident[16];
    uint16_t type, machine;

    uint32_t version,entry,phoff,shoff,flags;

    uint16_t flags,ehsize,phentsize,phnum,
    shentsize,shnum,shstrndx;

}__attribute__((packed)) Elf32_Ehdir;