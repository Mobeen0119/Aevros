#include <stdint.h>
#include "paging.h"
#include "pmm.c"
#include "../Process/task.h"

page_directory_t* kernel_directory = (page_directory_t*)0xFFFFF000;

uint32_t *get_virtual_table_address(uint32_t pd)
{
    return (uint32_t *)RECURSIVE_PT_BASE + (pd * 4096);
}

void map_page(uint32_t vir_addr, uint32_t phy_addr, uint32_t flags)
{
    uint32_t page_dir_index = vir_addr >> 22;
    uint32_t page_table_index = (vir_addr >> 12) & 0x03FF;

    uint32_t *page_dir = (uint32_t *)PAGE_RECURSIVE; // Acess to Page directory
    if (!(page_dir[page_dir_index]) && PAGE_PRESENT)
    {

        uint32_t new_page = pmm_alloc();
        page_dir[page_dir_index] = new_page | flags | PAGE_PRESENT; // Present + Read/Write

        memset(get_virtual_table_address(page_dir_index), 0, 4096);
    }

    uint32_t *page_table = get_virtual_table_address(page_table_index);
    page_table[page_table_index] = phy_addr | PAGE_PRESENT | PAGE_WRITE ;

    asm volatile("invlpg (%0)," ::"r"(vir_addr) : "memory");
}
