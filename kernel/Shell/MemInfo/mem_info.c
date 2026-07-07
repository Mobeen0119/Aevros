#include <stdbool.h>
#include "../../../Lib/kprintf.h"

#include "../../Memory/pmm.h"
#include "../../Memory/buddy.h"
#include "../../Memory/slab.h"

#include "../../Process/task.h"
#include "../../Memory/kheap.h"
#include "../../../Include/terminal.h"

#define ROW(label, fmt, ...) \
    kprintf(" %-28s " fmt "\n", label ":", ##__VA_ARGS__)

static char *fmt_bytes(uint32_t bytes, char *buf, int bufsz)
{
    if (bytes >= (1024 * 1024))
        ksnprintf(buf, bufsz, "%u MB (%u KB)", bytes / (1024 * 1024), bytes / 1024);

    else if (bytes >= 1024)
        ksnprintf(buf, bufsz, "%u KB", bytes / 1024);

    else
        ksnprintf(buf, bufsz, "%u B", bytes);

    return buf;
}

static uint32_t pct(uint32_t part, uint32_t total)
{
    return total ? (part * 100u) / total : 0;
}

static void print_bar(uint32_t used, uint32_t total, int width)
{
    uint32_t filled = total ? ((uint32_t)width * used) / total : 0;
    kprintf("[");
    for (int i = 0; i < width; i++)
    {
        kprintf(i < (int)filled ? "#" : ".");
    }
    kprintf("] %u%%", pct(used, total));
}

static void div_line(void)
{
    set_color(VGA_DARK_GREY, VGA_BLACK);
    kprintf("  +----------------------------------------------------------+\n");
    reset_color();
}

static void section(const char *title, const char *description)
{
    kprintf("\n");
    set_color(VGA_CYAN, VGA_BLACK);
    kprintf("  ===========================================================|\n");
    kprintf("  |  %-56s  |\n", title);
    reset_color();
    set_color(VGA_LIGHT_GREY, VGA_BLACK);
    kprintf("  |  %-56s  |\n", description);
    set_color(VGA_CYAN, VGA_BLACK);
    kprintf("  ===========================================================\n");
    reset_color();
}

void meminfo_pmm()
{
    section("PHYSICAL MEMORY  (PMM)",
            "Raw 4 KB frames across all of RAM");

    set_color(VGA_WHITE, VGA_BLACK);

    uint32_t total = pmm_total_frames();
    uint32_t free = pmm_free_frames();
    uint32_t used = pmm_used_frames();

    bool consistent = (used + free == total);

    char buf[32];

    ROW("Physical RAM total", "%s", fmt_bytes(total * 4096, buf, sizeof buf));
    ROW("Frames total", "%u frames  (each frame = 4 KB)", total);
    kprintf("\n");
    ROW("Frames in use", "%u  (%u%%)", used, pct(used, total));
    ROW("Frames free", "%u  (%u%%)", free, pct(free, total));
    kprintf("\n");

    kprintf("  Usage  ");
    print_bar(used, total, 40);
    kprintf("\n\n");

    ROW("Highest used address", "0x%08x", pmm_get_top());

    if (!consistent)
    {
        set_color(VGA_YELLOW, VGA_BLACK);
        kprintf("\n  [!] WARNING: used(%u) + free(%u) != total(%u) - "
                "possible PMM bitmap corruption\n",
                used, free, total);
    }

    reset_color();
}

void meminfo_heap(void)
{
    section("HEAP",
            "Routes to slab (<=64B) or buddy (>64B)");

    set_color(VGA_WHITE, VGA_BLACK);

    char buf_u[32], buf_f[32], buf_t[32];

    uint32_t used  = heap_used_bytes();
    uint32_t free  = heap_free_bytes();
    uint32_t total = heap_total_bytes();

    ROW("Total managed",  "%s", fmt_bytes(total, buf_t, sizeof buf_t));
    ROW("In use",         "%s  (%u%%)", fmt_bytes(used, buf_u, sizeof buf_u), pct(used, total));
    ROW("Free",           "%s  (%u%%)", fmt_bytes(free, buf_f, sizeof buf_f), pct(free, total));
    kprintf("\n");
    kprintf("  Usage  ");
    print_bar(used, total, 40);
    kprintf("\n\n");
    ROW("Largest free block", "%s", fmt_bytes(heap_largest_free_block(), buf_u, sizeof buf_u));
    ROW("Live allocations",   "%u", heap_live_allocation());

    reset_color();
}

void meminfo_buddy(void)
{
    section("BUDDY ALLOCATOR",
            "Manages above the PMM");

    set_color(VGA_WHITE, VGA_BLACK);

    uint32_t total = buddy_total_memory();
    uint32_t free_mem = buddy_free_memory();
    uint32_t used_mem = total - free_mem;

    bool fragmented = (free_mem > 0 && buddy_largest_free_block() < free_mem);
    char buf_t[32], buf_f[32], buf_u[32];

    ROW("Total managed",         "%s", fmt_bytes(total,    buf_t, sizeof buf_t));
    ROW("In use",                "%s  (%u%%)", fmt_bytes(used_mem, buf_u, sizeof buf_u), pct(used_mem, total));
    ROW("Free",                  "%s  (%u%%)", fmt_bytes(free_mem, buf_f, sizeof buf_f), pct(free_mem, total));

    kprintf("\n");

    kprintf(" Usage ");
    print_bar(used_mem, total, 40);
    kprintf("\n\n");
    kprintf("  Free blocks by order (order N = 2^N * 4KB block):\n");
    div_line();
    set_color(VGA_WHITE, VGA_BLACK);
    for (int order = 0; order <= MAX_ORDER; order++)
    {
        uint32_t block_size = ((uint32_t)1 << order) * 4096;
        uint32_t count = buddy_free_count_at_order(order);

        uint32_t order_bytes = count * block_size;
        if (count == 0)
            continue;

        char bsz[24], osz[24];
        kprintf("    order %2d  (%s/block)  %3u block%s  =  %s free\n",
                order,
                fmt_bytes(block_size, bsz, sizeof bsz),
                count, count == 1 ? " " : "s",
                fmt_bytes(order_bytes, osz, sizeof osz));
    }
    div_line();

    if (fragmented)
    {
        set_color(VGA_YELLOW, VGA_BLACK);
        kprintf("  [!] Fragmented: free memory exists but is split "
                "large allocations may fail.\n");
    }

    reset_color();
}

void meminfo_slab_cache(const char *label, slab_t *cache,
                                uint32_t obj_size){

    uint32_t used=0;
    uint32_t bm=cache->bitmap;

    for(int k=0;k<32;k++)
        if(bm&(1u<<k)) used++;
    uint32_t free_objs=32-used;

    kprintf("  %-10s  %3u B/obj    %2u used   %2u free   ",
            label, obj_size, used, free_objs);
    print_bar(used, 32, 20);
    kprintf("\n");
   
}
void meminfo_slab(void)
{
    section("SLAB ALLOCATOR",
            "Fixed-size object pools: fast O(1) small allocations");

    set_color(VGA_WHITE, VGA_BLACK);

    uint32_t total_slots = 32 * 3;
    uint32_t used        = slab_objects_used();
    uint32_t free_objs   = total_slots - used;
 
    ROW("Total object slots",    "%u  (3 caches x 32 slots)", total_slots);
    ROW("Slots in use",          "%u  (%u%%)", used,      pct(used,      total_slots));
    ROW("Slots free",            "%u  (%u%%)", free_objs, pct(free_objs, total_slots));
    kprintf("\n");
 
    kprintf("  Per-cache breakdown:\n");
    div_line();
    meminfo_slab_cache("cache_32b",  &cache_32b,  32);
    meminfo_slab_cache("cache_64b",  &cache_64b,  64);
    meminfo_slab_cache("cache_128b", &cache_128b, 128);
    div_line();

    reset_color();
}

void meminfo_task(void)
{
    section("TASKS",
            "All runnable kernel tasks and their memory context");
 
    set_color(VGA_WHITE, VGA_BLACK);

    if (!ready_queue)
    {
        kprintf("  No tasks in ready queue.\n");
        reset_color();
        return;
    }
 
    div_line();
    set_color(VGA_WHITE, VGA_BLACK);
    kprintf("  %-6s  %-12s  %-12s  %s\n",
            "PID", "STATE", "CR3 (page dir)", "Notes");
    div_line();
 
    task_t *t = ready_queue;
    do {
        const char *state_str;
        int state_color = VGA_WHITE;
        switch (t->state)
        {
            case TASK_RUNNING:  state_str = "RUNNING";  state_color = VGA_GREEN;  break;
            case TASK_READY:    state_str = "READY";    state_color = VGA_CYAN;   break;
            case TASK_ZOMBIE:   state_str = "ZOMBIE";   state_color = VGA_RED;    break;
            case TASK_BLOCKED:  state_str = "BLOCKED";  state_color = VGA_YELLOW; break;
            default:            state_str = "UNKNOWN";  state_color = VGA_WHITE;  break;
        }
 
        kprintf("  %-6u  ", t->pid);
        set_color(state_color, VGA_BLACK);
        kprintf("%-12s", state_str);
        set_color(VGA_WHITE, VGA_BLACK);
        kprintf("  0x%08x  %s\n",
                t->cr3,
                (t == current_task) ? "<-- currently executing" : "");
 
        t = t->next;
    } while (t != ready_queue);
 
    div_line();
    reset_color();
}
 
void meminfo_paging(void)
{
    section("PAGING",
            "Virtual memory mapping .... current page directory (CR3)");
 
    set_color(VGA_WHITE, VGA_BLACK);

    if (current_task)
    {
        ROW("Current task PID",  "%u",       current_task->pid);
        ROW("Page directory CR3","0x%08x",   current_task->cr3);
        kprintf("\n");
        kprintf("  CR3 is the physical address of this task's page directory.\n");
        kprintf("  Each task has its own, giving isolated virtual address spaces.\n");
    }
    else
    {
        kprintf("  No current task - running in early boot or idle context.\n");
        kprintf("  CR3 contains the kernel's own page directory.\n");
    }

    reset_color();
}
 
static void meminfo_summary(void)
{
    uint32_t phys_total = pmm_total_frames() * 4096;
    uint32_t phys_used  = pmm_used_frames()  * 4096;
    uint32_t phys_free  = pmm_free_frames()  * 4096;
 
    char t[32], u[32], f[32];
 
    kprintf("\n");
    set_color(VGA_MAGENTA, VGA_BLACK);
    kprintf("  ----------------------------------------------------------------\n");
    kprintf("  |                 MEMORY REPORT :: meminfo                    |\n");
    kprintf("  |                                                              |\n");
    reset_color();
    set_color(VGA_WHITE, VGA_BLACK);
    kprintf("  |  Physical RAM:  %-44s|\n", fmt_bytes(phys_total, t, sizeof t));
    kprintf("  |  In use:        %-44s|\n", fmt_bytes(phys_used,  u, sizeof u));
    kprintf("  |  Free:          %-44s|\n", fmt_bytes(phys_free,  f, sizeof f));
    kprintf("  |                                                              |\n");
    kprintf("  |  Overall usage  ");
    print_bar(phys_used, phys_total, 32);
    kprintf("  |\n");
    kprintf("  |                                                              |\n");
    set_color(VGA_LIGHT_GREY, VGA_BLACK);
    kprintf("  |  Layers:  PMM -> Buddy -> Slab -> Heap -> (kmalloc)          |\n");
    kprintf("  |           Each section below covers one layer.              |\n");
    set_color(VGA_MAGENTA, VGA_BLACK);
    kprintf("  +-------------------------------------------------------------+\n");
    reset_color();
}
 
void meminfo_all(void)
{
    meminfo_summary();   
 
    meminfo_pmm();       
    meminfo_buddy();    
    meminfo_slab();     
    meminfo_heap();     
    meminfo_task();     
    meminfo_paging();   
 
    set_color(VGA_GREEN, VGA_BLACK);
    kprintf("\n  End of memory report.\n\n");
    reset_color();
}
 