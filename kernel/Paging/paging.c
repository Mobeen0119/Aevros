#include <stdint.h>
#include "paging.h"
#include "../Memory/pmm.h"
#include "../../Lib/kprintf.h"
#include "../Process/task.h"


static uint32_t page_directory[1024] __attribute__((aligned(4096)));
static uint32_t first_page_table[1024]  __attribute__((aligned(4096)));
static uint32_t second_page_table[1024] __attribute__((aligned(4096)));  // ADD
static uint32_t third_page_table[1024]  __attribute__((aligned(4096)));  // ADD
static uint32_t fourth_page_table[1024] __attribute__((aligned(4096)));  // ADD

void paging_init() {
    for (int i = 0; i < 1024; i++)
        page_directory[i] = 0;

    for (int i = 0; i < 1024; i++)
        first_page_table[i] = (i * 0x1000) | PAGE_PRESENT | PAGE_WRITE;
    page_directory[0] = (uint32_t)first_page_table | PAGE_PRESENT | PAGE_WRITE;

    for (int i = 0; i < 1024; i++)
        second_page_table[i] = (0x400000 + i * 0x1000) | PAGE_PRESENT | PAGE_WRITE;
    page_directory[1] = (uint32_t)second_page_table | PAGE_PRESENT | PAGE_WRITE;

    
    for (int i = 0; i < 1024; i++)
        third_page_table[i] = (0x800000 + i * 0x1000) | PAGE_PRESENT | PAGE_WRITE;
    page_directory[2] = (uint32_t)third_page_table | PAGE_PRESENT | PAGE_WRITE;

  
    for (int i = 0; i < 1024; i++)
        fourth_page_table[i] = (0xC00000 + i * 0x1000) | PAGE_PRESENT | PAGE_WRITE;
    page_directory[3] = (uint32_t)fourth_page_table | PAGE_PRESENT | PAGE_WRITE;

    
    page_directory[1023] = (uint32_t)page_directory | PAGE_PRESENT | PAGE_WRITE;

    asm volatile("mov %0, %%cr3" :: "r"((uint32_t)page_directory) : "memory");

    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    asm volatile("mov %0, %%cr0" :: "r"(cr0) : "memory");
}

void map_page(uint32_t virt, uint32_t phys, uint32_t flags) {
    uint32_t pd_idx = virt >> 22;
    uint32_t pt_idx = (virt >> 12) & 0x3FF;

    uint32_t *pd = (uint32_t *)0xFFFFF000;

    if (!(pd[pd_idx] & PAGE_PRESENT)) {
        uint32_t new_pt = pmm_alloc();
        pd[pd_idx] = new_pt | PAGE_PRESENT | PAGE_WRITE;
        uint32_t *pt = (uint32_t *)(0xFFC00000 + pd_idx * 0x1000);
        for (int i = 0; i < 1024; i++) pt[i] = 0;
    }

    uint32_t *pt = (uint32_t *)(0xFFC00000 + pd_idx * 0x1000);
    pt[pt_idx] = phys | flags | PAGE_PRESENT;
    asm volatile("invlpg (%0)" :: "r"(virt) : "memory");
}

void unmap(uint32_t virt) {
    uint32_t pd_idx = virt >> 22;
    uint32_t pt_idx = (virt >> 12) & 0x3FF;

    uint32_t *pd = (uint32_t *)0xFFFFF000;
    if (!(pd[pd_idx] & PAGE_PRESENT)) return;

    uint32_t *pt = (uint32_t *)(0xFFC00000 + pd_idx * 0x1000);
    pt[pt_idx] = 0;
    asm volatile("invlpg (%0)" :: "r"(virt) : "memory");
}

uint32_t *get_virtual_table_address(uint32_t pd)
{
    return (uint32_t *)RECURSIVE_PT_BASE + (pd * 0x1000);
}

void *alloc_page_aligned()
{
    uint32_t phy = pmm_alloc();

    if (!phy)
        return VFS_OK;

    return (void *)(phy + 0xC0000000);
}

void memcpy_page_physical(uint32_t dst, uint32_t src)
{
    map_page(TEMP_SRC_PAGE, src, PAGE_PRESENT | PAGE_WRITE);
    map_page(TEMP_DST_PAGE, dst, PAGE_PRESENT | PAGE_WRITE);

    memcpy((void *)TEMP_DST_PAGE, (void *)TEMP_SRC_PAGE, 4096);

    unmap(TEMP_SRC_PAGE);
    unmap(TEMP_DST_PAGE);
}

uint32_t clone_page_directory(uint32_t src_cr3)
{

    (void *)src_cr3;

    uint32_t *current_pd = (uint32_t *)PAGE_RECURSIVE;

    uint32_t *new_pd = (uint32_t *)alloc_page_aligned();

    if (!new_pd)
        return VFS_OK;

    memset(new_pd, 0, 4096);

    for (int i = 768; i < 1024; i++)
    {
        new_pd[i] = current_pd[i];
    }

    for (int pd = 0; pd < 768; pd++)
    {
        if (!(current_pd[pd] & PAGE_PRESENT))
            continue;

        uint32_t *src_pt = (uint32_t *)(RECURSIVE_PT_BASE + (pd * 0x1000));

        uint32_t *new_pt = (uint32_t *)alloc_page_aligned();

        if (!new_pt)
            return VFS_OK;

        memset(new_pt, 0, 4096);

        uint32_t new_pt_phys = ((uint32_t)new_pt - 0xC0000000);

        new_pd[pd] = new_pt_phys | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;

        for (int pt = 0; pt < 1024; pt++)

        {
            if (!(src_pt[pt] & PAGE_PRESENT))
                continue;

            uint32_t src_phy = src_pt[pt] & 0xFFFFF000;

            uint32_t new_phy = pmm_alloc();

            if (!new_phy)
                return VFS_OK;

            memcpy_page_physical(new_phy, src_phy);

            new_pt[pt] = new_phy | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
        }
    }
    uint32_t new_pd_phys = ((uint32_t)new_pd - 0xC0000000);

    new_pd[1023] = new_pd_phys | PAGE_PRESENT | PAGE_WRITE;

    return new_pd_phys;
}

int map_page_in_directory(uint32_t tar_cr3, uint32_t vir,
                          uint32_t phy, uint32_t flags)
{

    uint32_t pd_index = vir >> 22;
    uint32_t pt_index = (vir >> 12) & 0x3FF;

    map_page(TEMP_PD_VIRT, tar_cr3, PAGE_PRESENT | PAGE_WRITE);

    uint32_t *tar_pd = (uint32_t *)TEMP_PD_VIRT;

    if (!(tar_pd[pd_index] & PAGE_PRESENT))
    {
        uint32_t new_pt_phy = pmm_alloc();

        if (!new_pt_phy)
            return 1;

        map_page(TEMP_SRC_PAGE, new_pt_phy, PAGE_PRESENT | PAGE_WRITE);

        memset((void *)TEMP_SRC_PAGE, 0, 4096);

        tar_pd[pd_index] = new_pt_phy | flags | PAGE_PRESENT;

        uint32_t pt_phy = tar_pd[pd_index] & 0xFFFFF000;

        map_page(TEMP_SRC_PAGE, pt_phy, PAGE_PRESENT | PAGE_WRITE);

        uint32_t *tar_pt = (uint32_t *)TEMP_SRC_PAGE;

        tar_pt[pt_index] = phy | flags | PAGE_PRESENT;

        unmap(TEMP_PD_VIRT);
        unmap(TEMP_SRC_PAGE);
    }
    return 1;
}