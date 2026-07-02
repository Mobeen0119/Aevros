#include <stddef.h>

#include "elf.h"
#include "../Memory/pmm.h"
#include "../Paging/paging.h"
#include "../../Lib/string.h"

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

int elf_load_segs(Elf32_Header *hdr, uint32_t target_cr3)
{
    if (!hdr || !target_cr3)
        return 0;

    ELF32_Phdr *phdrs = elf_prog_headers(hdr);
    if (!phdrs)
        return 0;

    for (int i = 0; i < hdr->program_header_count; i++)
    {
        ELF32_Phdr *ph = &phdrs[i];

        if (ph->type != PT_LOAD)
            continue;

        uint32_t start = PAGE_ALIGN_DOWN(ph->vir_address);
        uint32_t end   = PAGE_ALIGN_UP(ph->vir_address + ph->mem_size);

        uint32_t src_off = ph->offset;
        uint32_t file_remaining = ph->file_size;

        for (uint32_t addr = start; addr < end; addr += PAGE_SIZE)
        {
            uint32_t phys = pmm_alloc();
            if (!phys)
                return 0;

            map_page_in_directory(target_cr3, addr, phys,
                                  PAGE_PRESENT | PAGE_WRITE | PAGE_USER);

            map_page(TEMP_MAP_VIRT, phys, PAGE_PRESENT | PAGE_WRITE);

            uint8_t *dst = (uint8_t *)TEMP_MAP_VIRT;

            uint32_t page_virt_start = addr;
            uint32_t seg_virt_start  = ph->vir_address;

            uint32_t copy_start = (page_virt_start < seg_virt_start)
                                      ? (seg_virt_start - page_virt_start)
                                      : 0;

            uint32_t copy_bytes = 0;
            if (file_remaining > 0 && copy_start < PAGE_SIZE)
            {
                copy_bytes = PAGE_SIZE - copy_start;
                if (copy_bytes > file_remaining)
                    copy_bytes = file_remaining;
            }

            for (uint32_t b = 0; b < PAGE_SIZE; b++)
                dst[b] = 0;

            if (copy_bytes > 0)
            {
                uint8_t *src = (uint8_t *)hdr + src_off;
                for (uint32_t b = 0; b < copy_bytes; b++)
                    dst[copy_start + b] = src[b];

                src_off += copy_bytes;
                file_remaining -= copy_bytes;
            }

            unmap(TEMP_MAP_VIRT);
        }
    }

    return 1;
}
