#include<stdint.h>
#define PAGE_PRESENT 0X1
#define PAGE_WRITE 0X2
#define PAGE_USER 0X4

#define PAGE_RECURSIVE (uint32_t*)0xFFFFF000   // page directory
#define RECURSIVE_PT_BASE (uint32_t*)0xFFC00000  //starting address of all page table 

uint32_t* get_virtual_table_address(uint32_t pd_in);
void map_page(uint32_t vir_addr, uint32_t phy_addr, uint32_t flags);

