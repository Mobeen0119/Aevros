#include <stdbool.h>
#include "../../../Lib/kprintf.h"

#include "../../Memory/pmm.h"
#include "../../Memory/buddy.h"
#include "../../Memory/slab.h"

#include "../../Process/task.h"

#define ROW(label, fmt, ...) \
    kprintf(" %-28s " fmt "\n", label ":", ##__VA_ARGS__)

static char *fmt_bytes(uint32_t bytes, char *buf, int bufsz)
{
    if (bytes >= (1024 * 1024))
        kprintf(buf, bufsz, "%u MB (%u KB)", bytes / (1024 * 1024), bytes / 1024);

    else if (bytes >= 1024)
        kprintf(buf, bufsz, "%U KB", bytes / 1024);

    else
        kprintf(buf, bufsz, "%u B", bytes);

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
    kprintf("  +----------------------------------------------------------+\n");
}

static void section(const char *title, const char *description)
{
    kprintf("\n");
    kprintf("  ╔══════════════════════════════════════════════════════════╗\n");
    kprintf("  ║  %-56s  ║\n", title);
    kprintf("  ║  %-56s  ║\n", description);
    kprintf("  ╚══════════════════════════════════════════════════════════╝\n");
}

void meminfo_pmm()
{
    section("PHYSICAL MEMORY  (PMM)",
            "Raw 4 KB frames across all of RAM");

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
        kprintf("\n  [!] WARNING: used(%u) + free(%u) != total(%u) - "
                "possible PMM bitmap corruption\n",
                used, free, total);
}

void meminfo_heap(void)
{
    kprintf("\n==== Heap ====\n");
}

void meminfo_paging(void)
{
    kprintf("\n=== PAGING ===\n");

    if (current_task)
        kprintf("Current CR3 : 0x%x\n", current_task->cr3);
    else
        kprintf("No current task\n");
}

void meminfo_task(void)
{
    kprintf("\n=== Task ===\n");

    if (!ready_queue)
    {
        kprintf("No tasks\n");
        return;
    }

    task_t *t = ready_queue;

    do
    {
        kprintf(
            "PID=%u STATE=%u CR3=0x%x\n",
            t->pid,
            t->state,
            t->cr3);

        t = t->next;

    } while (t != ready_queue);
}

void meminfo_buddy(void)
{
    section("BUDDY ALLOCATOR",
            "Manages above the PMM");

    uint32_t total = buddy_total_memory();
    uint32_t free_mem = buddy_free_memory();
    uint32_t used_mem = total - free_mem;

    bool fragmented = (free_mem > 0 && buddy_largest_free_block() < free_mem);
    char buf_t[32], buf_f[32], buf_u[32];

    ROW("Total managed", "%s (%u%%)",
        fmt_bytes(used_mem, buf_u, sizeof(buf_u)), pct(used_mem, total));

    kprintf("\n");

    kprintf(" Usage ");
    print_bar(used_mem, total, 40);
    kprintf("\n\n");
    kprintf("  Free blocks by order (order N = 2^N * 4KB block):\n");
    div_line();
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
        kprintf("  [!] Fragmented: free memory exists but is split "
                "large allocations may fail.\n");
}

void meminfo_slab()
{
    kprintf("\n=== SLAB ===\n");

    kprintf("Free Objects : &u\n", slab_objects_free());
    kprintf("Used Objects : &u\n", slab_objects_used());
}

void meminfo_all()
{
    meminfo_pmm();

    meminfo_buddy();

    meminfo_slab();

    meminfo_heap();

    meminfo_task();

    meminfo_paging();
}