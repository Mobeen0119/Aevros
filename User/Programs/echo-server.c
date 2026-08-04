typedef unsigned int uint32_t;

static inline int syscall3(int num, int a1, int a2, int a3)
{

    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(ret), "b"(a1), "c"(a2), "d"(a3));
}

#define SYS_WRITE 1
#define SYS_EXIT 6
#define SYS_SOCKET 9
#define SYS_BIND 10
#define SYS_ACCEPT 12
#define SYS_SEND 13
#define SYS_RECV 14
#define SYS_SOCKCLOSE 17
#define SOCK_TYPE_TCP 1
#define ECHO_PORT 7

static void print(const char *s)
{
    int len = 0;
    while (s[len])
        len++;

    syscall3(SYS_WRITE, 1, (int)s, len);
}

void _start(void)
{
    print("echo-server: starting\n");

    int listen_fd = syscall3(SYS_SOCKET, SOCK_TYPE_TCP, 0, 0);

    if (listen_fd < 0)
    {
        print("echo-server: could not create sockets\n");
        syscall3(SYS_EXIT, 0, 0, 0);
        while (1)
        {
        }
    }

    if (syscall3(SYS_BIND, listen_fd, ECHO_PORT, 0) < 0)
    {
        print("echo-server: could not bind port 7\n");
        syscall3(SYS_EXIT, 0, 0, 0);
        while (1)
        {
        }
    }
    print("echo-server: listening on port 7\n");

    unsigned char buf[512];

    for (;;)
    {
        int conn_fd = syscall3(SYS_ACCEPT, listen_fd, 0, 0);

        if (conn_fd < 0)
            continue;

        print("echo-server: connection accepted\n");

        for (;;)
        {
            int got = syscall3(SYS_RECV, conn_fd, (int)buf, sizeof(buf));

            if (got > 0)
            {
                syscall3(SYS_SEND, conn_fd, (int)buf, got);
                continue;
            }
            if (got == -2)
            {
                print("echo-server: connection close\n");
                break;
            }
            syscall3(SYS_SOCKCLOSE, conn_fd, 0, 0);
        }
    }
}