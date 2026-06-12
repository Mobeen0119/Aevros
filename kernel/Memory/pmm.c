#include "pmm.h"


#define FRAME_SIZE 4096

uint32_t mem[1024];

uint32_t frame_stack[MAX_FRAMES];
static int32_t stack_top = -1;
static uint32_t total_frames = 0;

void pmm_init(uint32_t st_address, uint32_t size)
{
    memset(frame_stack, 0, sizeof(frame_stack));

    st_address = (st_address + FRAME_SIZE - 1) & ~(FRAME_SIZE - 1);
    
    for (uint32_t address = st_address; address < (st_address + size); address += FRAME_SIZE)
    {
        if (stack_top >= MAX_FRAMES - 1)
            break;

        frame_stack[++stack_top] = address;
        total_frames++;
    }
}

uint32_t pmm_alloc(void)
{
    if (stack_top < 0)
        return 0;

    return frame_stack[stack_top--];
}

void pmm_free(uint32_t address)
{
    if (!address)
        return;

    if (address & (FRAME_SIZE - 1))
        return;

    for (int32_t i = 0; i <= stack_top; i++)
    {
        if (frame_stack[i] == address)
            return;
    }

    if (stack_top >= MAX_FRAMES - 1)
        return;

    frame_stack[++stack_top] = address;
}

uint32_t pmm_free_frames(void)
{
    return stack_top + 1;
}

uint32_t pmm_get_top() {
    uint32_t highest = 0;
    for (int i = 0; i <= stack_top; i++)
        if (frame_stack[i] > highest)
            highest = frame_stack[i];
    return highest + FRAME_SIZE;
}

uint32_t pmm_used_frames(void)
{
    return total_frames - (stack_top + 1);
}

uint32_t pmm_total_frames(void)
{
    return total_frames;
}