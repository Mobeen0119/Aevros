#include "../../../Lib/kprintf.h"

#include "../../Memory/pmm.h"
#include "../../Memory/buddy.h"
#include "../../Memory/slab.h"

#include "../../Process/task.h"


void meminfo_pmm()
{
    kprintf("\n==== PMM ====\n");

    kprintf("Total Frames : &u\n", pmm_total_frames());
    kprintf("Free Frames : &u\n", pmm_free_frames());
    kprintf("Used Frames : &u\n", pmm_used_frames());

    kprintf("Top Address : &u\n", pmm_get_top());
}

void meminfo_heap()
{
    kprintf("\n==== Heap ====\n");
}

void meminfo_paging()
{
    kprintf("\n=== PAGING ===\n");

    if (current_task)
        kprintf("Current CR3 : 0x%x\n", current_task->cr3);
    else
        kprintf("No current task\n");
}

void meminfo_task()
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

void meminfo_buddy()
{
    kprintf("\n=== BUDDY ===\n");
   
    kprintf("Total Memory : &u\n", buddy_total_memory());
    kprintf("Free Memory : &u\n", buddy_free_memory());
    kprintf("Fragmentation : &u\n", buddy_fragmentation());

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