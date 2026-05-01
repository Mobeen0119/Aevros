#include <stdint.h>

typedef struct Process
{
    uint32_t id, esp, ebp,eip, page_directory;
    struct Process *next;
} task_t;