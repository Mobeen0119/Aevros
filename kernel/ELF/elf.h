#pragma once

#include <stdint.h>

#define ELF_MAGIC_NUMBER 0x464C457F
#define PT_LOAD 1

typedef struct
{

    uint32_t type, offset, paddr, 
    vaddr, filesz, memsz, flags, align;

} __attribute__((packed)) ELF32_Phdr;

typedef struct
{
    uint8_t identity[16];

    uint16_t file_type,architecture;

    uint32_t elf_version,entry_point;

    uint32_t program_header_offset,section_header_offset;

    uint32_t flags;

    uint16_t elf_header_size;

    uint16_t program_header_entry_size,program_header_count;

    uint16_t section_header_entry_size,section_header_count;

    uint16_t section_string_table_index;

} __attribute__((packed)) Elf32_Header;

Elf32_Header*  elf_prog_header(Elf32_Header* hdr);
int elf_load_seg(Elf32_Header* hdr);