#include "../Lib/kprintf.h"
#include "../Lib/string.h"
#include "Process/task.h"
#include "Memory/buddy.h"
#include "Memory/kheap.h"
#include "Syscall/syscall.h"
#include "Process/exec.h"
#include "selftest.h"
#include "../Lib/string.h"
#include "../Include/shell.h"


static int pass = 0, fail = 0;
#define CHECK(name, cond) do { \
    if (cond) { kprintf("  [PASS] %s\n", name); pass++; } \
    else      { kprintf("  [FAIL] %s\n", name); fail++; } \
} while (0)


static int buddy_list_is_finite(int order, uint32_t limit)
{
    extern buddy_block_t *free_lists[];
    buddy_block_t *b = free_lists[order];
    uint32_t count = 0;
    while (b && count < limit) { b = b->next; count++; }
    return count < limit;
}

static void test_buddy(void)
{
    kprintf("-- buddy --\n");
    for (int o = 0; o <= MAX_ORDER; o++)
        CHECK("buddy list terminates", buddy_list_is_finite(o, 100000));
    void *a = buddy_alloc(0);
    CHECK("buddy_alloc(0) non-null", a != NULL);
    if (a) { buddy_free(a, 0); CHECK("buddy list still finite after free", buddy_list_is_finite(0, 100000)); }
}

static void test_heap(void)
{
    kprintf("-- heap --\n");
    void *p = kmalloc_raw(64);
    CHECK("kmalloc_raw(64) non-null", p != NULL);
    if (p) { memset(p, 0xAA, 64); CHECK("write to allocated block doesn't fault", 1); kfree_raw(p); }
}

static uint32_t count_ready_queue(uint32_t limit, int *ok)
{
    if (!ready_queue) { *ok = 1; return 0; }
    task_t *t = ready_queue; uint32_t count = 0;
    do { count++; t = t->next; } while (t != ready_queue && count < limit);
    *ok = (count < limit);
    return count;
}

static void test_task_list(void)
{
    kprintf("-- task list --\n");
    int ok; count_ready_queue(1000, &ok);
    CHECK("ready_queue terminates (no cycle)", ok);
}

static void test_tokenize(void)
{
    kprintf("-- tokenize --\n");
    char buf1[] = "ls"; char *argv[MAX_ARG];
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
    task_t fake; memset(&fake, 0, sizeof(fake));
    task_t *saved = current_task; current_task = &fake;
    int r = sys_write(1, (uint8_t*)"x", 1);
    CHECK("sys_write on unopened fd doesn't crash, returns error", r < 0);
    current_task = saved;
}

static void test_syscall_dispatch(void)
{
    kprintf("-- syscall dispatch --\n");
    register_t r; memset(&r, 0, sizeof(r));
    r.eax = 99;
    extern void syscall_handler(register_t *regs);
    syscall_handler(&r);
    CHECK("unknown syscall number returns -1 in eax, no crash", r.eax == (uint32_t)-1);
}

static void test_fork(void)
{
    kprintf("-- fork --\n");
    kprintf("  [SKIP] fork needs a real interrupt frame (live esp/ebp mid-stack).\n");
    kprintf("  [SKIP] Use the 'forktest' user program + ps/timeline instead.\n");
}

void selftest_run(void)
{
    pass = 0; fail = 0;
    kprintf("\n=== SELFTEST (combined) ===\n");
    test_buddy();
    test_heap();
    test_task_list();
    test_tokenize();
    test_fd_safety();
    test_syscall_dispatch();
    test_fork();
    kprintf("\n%d passed, %d failed\n\n", pass, fail);
}