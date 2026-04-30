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

