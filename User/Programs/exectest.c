typedef unsigned int uint32_t;

static inline int syscall3(int num, int a1, int a2, int a3)
{
    int ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(num), "b"(a1), "c"(a2), "d"(a3));
    return ret;
}

void _start(void)
{
    const char msg[] = "exec worked\n";
    syscall3(1, 1, (int)msg, sizeof(msg) - 1);
    syscall3(6, 0, 0, 0);
    while (1) {}
}
