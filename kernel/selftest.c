#include "../Lib/kprintf.h"
#include "../Lib/string.h"
#include "Process/task.h"
#include "Memory/buddy.h"
#include "Memory/kheap.h"
#include "Syscall/syscall.h"
#include "Process/exec.h"
#include "selftest.h"
#include "../Drivers/PIT/pit.h"
#include "../Lib/string.h"
#include "../Include/shell.h"

static int pass = 0, fail = 0;
#define CHECK(name, cond)                   \
    do                                      \
    {                                       \
        if (cond)                           \
        {                                   \
            kprintf("  [PASS] %s\n", name); \
            pass++;                         \
        }                                   \
        else                                \
        {                                   \
            kprintf("  [FAIL] %s\n", name); \
            fail++;                         \
        }                                   \
    } while (0)

static int buddy_list_is_finite(int order, uint32_t limit)
{
    extern buddy_block_t *free_lists[];
    buddy_block_t *b = free_lists[order];
    uint32_t count = 0;
    while (b && count < limit)
    {
        b = b->next;
        count++;
    }
    return count < limit;
}

static void test_buddy(void)
{
    kprintf("-- buddy --\n");
    for (int o = 0; o <= MAX_ORDER; o++)
        CHECK("buddy list terminates", buddy_list_is_finite(o, 100000));
    void *a = buddy_alloc(0);
    CHECK("buddy_alloc(0) non-null", a != NULL);
    if (a)
    {
        buddy_free(a, 0);
        CHECK("buddy list still finite after free", buddy_list_is_finite(0, 100000));
    }
}

static void test_heap(void)
{
    kprintf("-- heap --\n");
    void *p = kmalloc_raw(64);
    CHECK("kmalloc_raw(64) non-null", p != NULL);
    if (p)
    {
        memset(p, 0xAA, 64);
        CHECK("write to allocated block doesn't fault", 1);
        kfree_raw(p);
    }
}

static uint32_t count_ready_queue(uint32_t limit, int *ok)
{
    if (!ready_queue)
    {
        *ok = 1;
        return 0;
    }
    task_t *t = ready_queue;
    uint32_t count = 0;
    do
    {
        count++;
        t = t->next;
    } while (t != ready_queue && count < limit);
    *ok = (count < limit);
    return count;
}

static void test_task_list(void)
{
    kprintf("-- task list --\n");
    int ok;
    count_ready_queue(1000, &ok);
    CHECK("ready_queue terminates (no cycle)", ok);
}

static void test_tokenize(void)
{
    kprintf("-- tokenize --\n");
    char buf1[] = "ls";
    char *argv[MAX_ARG];
    int argc = tokenize(buf1, argv);
    CHECK("tokenize('ls') argc==1", argc == 1);
    CHECK("tokenize('ls') argv[0]=='ls'", argc == 1 && strcmp(argv[0], "ls") == 0);
    char buf2[] = "echo hi there";
    argc = tokenize(buf2, argv);
    CHECK("tokenize('echo hi there') argc==3", argc == 3);
    CHECK("tokenize argv[1]=='hi'", argc >= 2 && strcmp(argv[1], "hi") == 0);
}

static void test_fd_safety(void)
{
    kprintf("-- fd safety --\n");
    task_t fake;
    memset(&fake, 0, sizeof(fake));
    task_t *saved = current_task;
    current_task = &fake;
    int r = sys_write(1, (uint8_t *)"x", 1);
    CHECK("sys_write on unopened fd doesn't crash, returns error", r < 0);
    current_task = saved;
}

static void test_syscall_dispatch(void)
{
    kprintf("-- syscall dispatch --\n");
    register_t r;
    memset(&r, 0, sizeof(r));
    r.eax = 99;
    extern void syscall_handler(register_t * regs);
    syscall_handler(&r);
    CHECK("unknown syscall number returns -1 in eax, no crash", r.eax == (uint32_t)-1);
}

static void test_fork(void)
{
    kprintf("-- fork --\n");
    kprintf("  [SKIP] fork needs a real interrupt frame (live esp/ebp mid-stack).\n");
    kprintf("  [SKIP] Use the 'forktest' user program + ps/timeline instead.\n");
}

static volatile uint32_t spin_a_count, spin_b_count;
static volatile int spin_stop;

static void spin_a(void)
{
    while (!spin_stop)
        spin_a_count++;
    sys_exit(0);
}

static void spin_b(void)
{
    while (!spin_stop)
        spin_b_count++;
    sys_exit(0);
}

static void busy_wait_ticks(uint32_t n)
{
    uint32_t start = get_ticks();
    while (get_ticks() - start < n)
        ;
}

static void test_preemption(void)
{
    kprintf("-- scheduler (timer-driven preemption, about 4s) --\n");

    spin_a_count = 0;
    spin_b_count = 0;
    spin_stop = 0;

    task_t *a = task_create_kernel(spin_a);
    task_t *b = task_create_kernel(spin_b);
    CHECK("two spinning tasks created", a && b);
    if (!a || !b)
        return;

    int pid_a = (int)a->pid, pid_b = (int)b->pid;

    // task also spins.... so a and b only run when the timer takes  CPU away from it
    busy_wait_ticks(200);
    uint32_t a1 = spin_a_count, b1 = spin_b_count;
    busy_wait_ticks(200);
    uint32_t a2 = spin_a_count, b2 = spin_b_count;

    spin_stop = 1;
    int st;
    sys_waitpid(pid_a, &st);
    sys_waitpid(pid_b, &st);

    kprintf("  counters at 2s: a=%u b=%u, at 4s: a=%u b=%u\n", a1, b1, a2, b2);
    CHECK("task A ran without yielding", a1 > 0);
    CHECK("task B ran without yielding", b1 > 0);
    CHECK("task A kept advancing (got the CPU again)", a2 > a1);
    CHECK("task B kept advancing (got the CPU again)", b2 > b1);
}

// two tasks hammer allocator while timer preempts
static volatile uint32_t heap_ops_a, heap_ops_b, heap_bad;
static volatile int heap_stop;

static void heap_worker(volatile uint32_t *ops, uint8_t tag)
{
    while (!heap_stop)
    {
        uint8_t *p = (uint8_t *)kmalloc(48);
        if (!p)
            continue;
        for (int i = 0; i < 48; i++)
            p[i] = tag;
        for (int spin = 0; spin < 200; spin++)
            asm volatile("" ::: "memory");
        for (int i = 0; i < 48; i++)
            if (p[i] != tag)
            {
                heap_bad++;
                break;
            }
        kfree(p);
        (*ops)++;
    }
    sys_exit(0);
}

static void heap_a(void) { heap_worker(&heap_ops_a, 0xA1); }
static void heap_b(void) { heap_worker(&heap_ops_b, 0xB2); }

static void test_heap_preempt(void)
{
    kprintf("-- allocator under preemption (about 4s, 10 ms time slices) --\n");
    heap_ops_a = heap_ops_b = heap_bad = 0;
    heap_stop = 0;
    task_t *a = task_create_kernel(heap_a);
    task_t *b = task_create_kernel(heap_b);
    if (!a || !b)
        return;
    int pa = (int)a->pid, pb = (int)b->pid;
    pit_set_time_slice(1); // about 400 context switches
    busy_wait_ticks(400);
    pit_set_time_slice(0);
    heap_stop = 1;
    int st;
    sys_waitpid(pa, &st);
    sys_waitpid(pb, &st);
    kprintf("  a did %u ops, b did %u ops, corrupted blocks seen: %u\n", heap_ops_a, heap_ops_b, heap_bad);
    CHECK("no block was corrupted while tasks were preempted", heap_bad == 0);
}

void selftest_run(void)
{
    pass = 0;
    fail = 0;
    kprintf("\n=== SELFTEST (combined) ===\n");
    test_buddy();
    test_heap();
    test_task_list();
    test_tokenize();
    test_fd_safety();
    test_syscall_dispatch();
    test_fork();
    test_preemption();
    test_heap_preempt();
    kprintf("\n%d passed, %d failed\n\n", pass, fail);
}