#include "process_memory.h"
#include "../../Paging/paging.h"
#include "../../Memory/pmm.h"
#include "../../../Lib/string.h"

uint32_t create_user_space(void)
{
    uint32_t pd_phy = pmm_alloc();
    if (!pd_phy) return 0;

    map_page(TEMP_PD_VIRT, pd_phy, PAGE_PRESENT | PAGE_WRITE);
    uint32_t *new_pd    = (uint32_t *)TEMP_PD_VIRT;
    uint32_t *kernel_pd = (uint32_t *)PAGE_RECURSIVE;

    memset(new_pd, 0, 4096);

    for (uint32_t i = 768; i < 1023; i++)
        new_pd[i] = kernel_pd[i];

    new_pd[0] |= PAGE_USER;

    new_pd[1023] = pd_phy | PAGE_PRESENT | PAGE_WRITE;
    unmap(TEMP_PD_VIRT);
    return pd_phy;
}

void destroy_user_space(uint32_t cr3)
{
    if (!cr3)
        return;

    map_page(TEMP_PD_VIRT, cr3, PAGE_PRESENT | PAGE_WRITE);

    uint32_t *pd = (uint32_t *)TEMP_PD_VIRT;

    for (uint32_t pd_index = 0; pd_index < 768; pd_index++)
    {
        if (!(pd[pd_index] & PAGE_PRESENT))
            continue;

        uint32_t pt_phy = pd[pd_index] & 0xFFFFF000;

        map_page(TEMP_PT_VIRT, pt_phy, PAGE_PRESENT | PAGE_WRITE);

        uint32_t *pt = (uint32_t *)TEMP_PT_VIRT;

        for (uint32_t pt_index = 0; pt_index < 1024; pt_index++)
        {
            if (!(pt[pt_index] & PAGE_PRESENT))
                continue;

            uint32_t phy_frame = pt[pt_index] & 0xFFFFF000;

            pmm_free(phy_frame);

            pt[pt_index] = 0;
        }
        unmap(TEMP_PT_VIRT);
        pmm_free(pt_phy);

        pd[pd_index] = 0;
    }
    unmap(TEMP_PD_VIRT);

    pmm_free(cr3);
}
