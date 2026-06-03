#include "process_memory.h"
#include "../../Paging/paging.h"
#include "../../Memory/pmm.h"

void destroy_user_space(uint32_t cr3)
{
    uint32_t *pd = get_virtual_table_address(cr3);

    for (uint32_t i = 0; i < 768; i++)
    {
        if (pd[i] & PAGE_PRESENT)
        {

            uint32_t *page_physical = pd[i] & 0xFFFFF000;

            uint32_t *page_table = get_virtual_table_address(page_physical);

            for (uint32_t j = 0; j < 1024; j++)
            {
                if (page_table[j] & PAGE_PRESENT)
                {
                    uint32_t *phy_frame = page_table[j] & 0xFFFFF000;

                    pmm_free(phy_frame);
                }
            }

            pmm_free(page_physical);

            pd[i] = 0;
        }
    }

    pmm_free(cr3);
}