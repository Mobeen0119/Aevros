#include <stdint.h>

typedef struct
{
    uint32_t entries[1024];
} page_directory_t;

typedef struct Process
{
    uint32_t id, esp, ebp, eip, page_directory;
    struct Process *next;
    uint32_t kernel_directory_phys;
} task_t;