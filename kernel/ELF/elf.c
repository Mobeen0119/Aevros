#include <stddef.h>

#include "elf.h"
#include "../Memory/pmm.h"
#include "../Paging/paging.h"

int elf_validate(Elf32_Header *hdr)
{

    if (!hdr)
        return 0;

    uint32_t *magic = (uint32_t *)hdr->identity;

    if (*magic != ELF_MAGIC_NUMBER)
        return 0;

    return 1;
}

ELF32_Phdr *elf_prog_headers(Elf32_Header *hdr)
{
    if (!hdr)
        return NULL;

    return (ELF32_Phdr *)((uint8_t *)hdr + hdr->program_header_offset);
}

int elf_load_segs(Elf32_Header *hdr,uint32_t target_cr3)
{
    if (!hdr)
        return 0;

    ELF32_Phdr *phdrs = elf_prog_headers(hdr);

    for (int i = 0; i < hdr->program_header_count; i++)
    {
        ELF32_Phdr *ph = &phdrs[i];

        if (ph->type != PT_LOAD)
            continue;

        uint32_t start = PAGE_ALIGN_DOWN(ph->vir_address);
        uint32_t end = PAGE_ALIGN_UP(ph->vir_address + ph->mem_size);

        for (uint32_t addr = start; addr < end; addr += PAGE_SIZE)
        {
            uint32_t phys = pmm_alloc();

            if (!phys)
                return 0;

            map_page_in_directory(target_cr3, ph->vir_address,phys,
                     PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
        }
        memcpy((void *)ph->vir_address, (uint8_t *)hdr + ph->offset,
               ph->file_size);

        if (ph->mem_size > ph->file_size)
        {
            memset((void *)(ph->vir_address + ph->file_size),
                   0, ph->mem_size - ph->file_size);
        }
    }

    return 1;
}

