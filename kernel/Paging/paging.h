#include<stdint.h>
#define PAGE_PRESENT 0X1
#define PAGE_WRITE 0X2
#define PAGE_USER 0X4
#define PAGE_RECURSIVE (uint32_t*)0xFFFFF000   // page directory
#define RECURSIVE_PT_BASE (uint32_t*)0xFFC00000  //starting address of all page table 
#define TEMP_SRC_PAGE 0xFF800000
#define TEMP_DST_PAGE 0xFF801000



uint32_t* get_virtual_table_address(uint32_t pd_in);
void map_page(uint32_t vir_addr, uint32_t phy_addr, uint32_t flags);

uint32_t clone_page_directory(uint32_t src_cr3);

void *alloc_page_aligned();

void memcpy_page_physical(uint32_t dst, uint32_t src);