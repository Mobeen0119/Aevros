#include "pmm.h"
#define MAX_FRAMES 1024
#define FRAME_SIZE 4096

uint32_t frame_stack[MAX_FRAMES];
int32_t stack_top = -1;

void pmm_init(uint32_t st_address, uint32_t size)
{
    memset(frame_stack, 0, sizeof(frame_stack));

    for (uint32_t address = st_address; address < (st_address + size); address += FRAME_SIZE)
    {
        if (stack_top < MAX_FRAMES - 1)
        {
            stack_top++;
            frame_stack[stack_top] = address;
        }
    }
}

uint32_t pmm_alloc()
{
    if (stack_top == -1)
        return 0;

    uint32_t address = frame_stack[stack_top];
    stack_top--;
    return address;
}

void pmm_free(uint32_t address)
{
    if (stack_top < MAX_FRAMES - 1)
    {
        stack_top++;
        frame_stack[stack_top] = address;
    }
}

void map_page(uint32_t vir_addr, uint32_t phy_addr, uint32_t flags)
{

    uint32_t page_dir_index = vir_addr >> 22;
    uint32_t page_table_index = (vir_addr >> 22) & 0x03FF;

    uint32_t *page_dir = (uint32_t *)0xFFFFF000;    // Acess to Page directory
    if (!(page_dir[page_dir_index]) && 0x1)
    {

        uint32_t new_page = pmm_alloc();
        page_dir[page_dir_index] = new_page | 3;     // Present + Read/Write

        memset(get_virtual_table_address(page_dir_index), 0, 4096);
    }

    uint32_t *page_table = get_virtual_table_address(page_table_index);
    page_table[page_table_index] = phy_addr | 3;

    asm volatile("invlpg (%0)," ::"r"(vir_addr) : "memory");
}