#include <stdint.h>
#include "paging.h"
#include "pmm.h"
#include "../Process/task.h"

uint32_t *kernel_directory = (uint32_t*)0xFFFFF000;

uint32_t *get_virtual_table_address(uint32_t pd)
{
    return (uint32_t *)RECURSIVE_PT_BASE + (pd * 4096);
}

void map_page(uint32_t vir_addr, uint32_t phy_addr, uint32_t flags)
{
    uint32_t page_dir_index = vir_addr >> 22;
    uint32_t page_table_index = (vir_addr >> 12) & 0x03FF;

    uint32_t *page_dir = (uint32_t *)PAGE_RECURSIVE; // Acess to Page directory

    if (!(page_dir[page_dir_index] & PAGE_PRESENT))
    {

        uint32_t new_page = pmm_alloc();
        page_dir[page_dir_index] = new_page | flags | PAGE_PRESENT; // Present + Read/Write

        memset(get_virtual_table_address(page_dir_index), 0, 4096);
    }

    uint32_t *page_table = get_virtual_table_address(page_dir_index);
    page_table[page_table_index] = phy_addr | flags | PAGE_PRESENT;

    asm volatile("invlpg (%0)" ::"r"(vir_addr) : "memory");
}

void unmap(uint32_t vir_addr)
{
    uint32_t p_dir_index = vir_addr >> 22;
    uint32_t p_table_index = (vir_addr >> 12) & 0x03FF;

    uint32_t *p_dir = (uint32_t *)0xFFFFF000;

    if (!(p_dir[p_dir_index] & 0x1))
        return;

    uint32_t *p_table = (uint32_t *)(0xFFC00000 + (p_dir_index * 0x1000));

    if (!(p_table[p_table_index] & 0x1))
        return;

    p_table[p_table_index] = 0;

    asm volatile("invlpg (%0)" ::"r"(vir_addr) : "memory");
}

uint32_t clone_page_direcroy(){

    uint32_t *current_dir=(uint32_t*)0xFFFFF000;
    uint32_t* new_dir=kmalloc(4096);

    memset(new_dir,0,4096);

    for(int i=786;i<1024;i++){
        new_dir[i]=current_dir[i];
    }

    return new_dir;
}