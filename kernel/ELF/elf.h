#pragma once

#include <stdint.h>

#define ELF_MAGIC_NUMBER 0x464C457F
#define PT_LOAD 1
#define PT_NULL 0
#define PT_DYNAMIC 2
#define PT_INTERP 3

#define PAGE_SIZE 4096

#define PAGE_ALIGN_DOWN(x) \
((x) & ~(PAGE_SIZE - 1))

#define PAGE_ALIGN_DOWN(y) \
((x) + (PAGE_SIZE - 1)  &  ~(PAGE_SIZE - 1))

typedef struct
{

    uint32_t type, offset, phy_address, 
    vir_address, file_size, mem_size, flags, align;

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

ELF32_Phdr*  elf_prog_headers(Elf32_Header* hdr);
int elf_load_segs(Elf32_Header* hdr);